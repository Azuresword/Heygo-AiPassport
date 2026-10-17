// main/link/link_core.c
#include "link_core.h"

#include <string.h>

static void send(link_core_t *l, link_lane_t lane, const uint8_t *pkt, size_t len) {
    if (len && l->io.send) l->io.send(l->io.ctx, lane, pkt, len);
}

static bool can_send(const link_core_t *l) { return l->connected && l->subscribed; }

static void drop(link_core_t *l, const bp_packet_t *p) {
    const uint8_t current = p->type == BP_AUDIO_DOWN || p->type == BP_PLAY_END ? l->down_xid : l->agent_xid;
    if (l->io.dropped) l->io.dropped(l->io.ctx, p->type, p->xid, current);
}

static void close_down(link_core_t *l, uint8_t channel) {
    if (l->down_open && l->down_channel == channel) l->down_open = false;
}

static void send_error(link_core_t *l, uint8_t code, uint8_t ref_type, uint8_t ref_xid) {
    if (!can_send(l)) return;
    uint8_t pkt[8];
    send(l, LINK_LANE_CONTROL, pkt, bp_build_error(pkt, sizeof pkt, code, ref_type, ref_xid));
}

static void request_turns(link_core_t *l, uint32_t now_ms);

static void send_hello(link_core_t *l, uint32_t now_ms) {
    uint8_t pkt[BP_HEADER_SIZE + 5 + BP_VERSION_TEXT_MAX];
    send(l, LINK_LANE_CONTROL, pkt,
         bp_build_hello(pkt, sizeof pkt, BP_CODEC_MASK_ADPCM, l->mtu, l->fw_version));
    l->hello_sent = true;
    l->hello_ms = now_ms;
}

static void send_device_state(link_core_t *l, bool context, uint32_t now_ms) {
    if (!can_send(l) || !l->acked) return;
    uint8_t pkt[16];
    if (context) send(l, LINK_LANE_CONTROL, pkt, bp_build_context(pkt, sizeof pkt, l->mode, l->page));
    send(l, LINK_LANE_CONTROL, pkt,
         bp_build_device_status(pkt, sizeof pkt, l->battery, l->mode, l->page, l->free_kib));
    l->status_ms = now_ms;
}

static link_voice_t compute_voice(const link_core_t *l, uint32_t now_ms) {
    if (!can_send(l)) return LINK_VOICE_NO_LINK;
    if (l->version_bad) return LINK_VOICE_VERSION;
    if (!l->acked) {
        if (!l->hello_sent || (uint32_t)(now_ms - l->hello_ms) < LINK_HELLO_ACK_MS) return LINK_VOICE_WAIT_HELLO;
        return LINK_VOICE_NO_ACK;
    }
    if (l->mtu < BP_MTU_MIN) return LINK_VOICE_MTU;
    if ((uint32_t)(now_ms - l->last_rx_ms) >= LINK_PHONE_SILENT_MS) return LINK_VOICE_SILENT;
    return LINK_VOICE_OK;
}

// An exchange cannot outlive voice. Close it and tell the phone when the link still works.
static void close_exchange(link_core_t *l) {
    if (!l->agent_xid) return;
    if (l->io.drop_audio) l->io.drop_audio(l->io.ctx);
    if (can_send(l)) {
        uint8_t pkt[8];
        send(l, LINK_LANE_CONTROL, pkt, bp_build_agent_cancel(pkt, sizeof pkt, l->agent_xid));
    }
    l->agent_xid = 0;
    close_down(l, BP_CHANNEL_AGENT);
}

static void update_voice(link_core_t *l, uint32_t now_ms) {
    const link_voice_t v = compute_voice(l, now_ms);
    if (v == l->voice) return;
    l->voice = v;
    if (v != LINK_VOICE_OK) close_exchange(l);
    if (l->io.voice_changed) l->io.voice_changed(l->io.ctx, v);
}

void link_init(link_core_t *l, const link_io_t *io, const char *fw_version) {
    memset(l, 0, sizeof *l);
    if (io) l->io = *io;
    l->fw_version = fw_version ? fw_version : "";
    l->mtu = 23;
    l->voice = LINK_VOICE_NO_LINK;
    l->battery = BP_BATTERY_UNKNOWN;
    bp_xid_init_device(&l->xids);
}

void link_on_connect(link_core_t *l, uint32_t now_ms) {
    l->connected = true;
    l->subscribed = false;
    l->hello_sent = false;
    l->acked = false;
    l->version_bad = false;
    l->mtu = 23;
    l->agent_xid = 0;
    l->down_open = false;
    l->popup.xid = 0;
    l->last_rx_ms = now_ms;   // Defensive. The silence rule starts after HELLO_ACK, which also sets it.
    // Doc 07: a new connection resets every xid.
    bp_xid_init_device(&l->xids);
    update_voice(l, now_ms);
}

void link_on_disconnect(link_core_t *l, uint32_t now_ms) {
    l->connected = false;
    l->subscribed = false;
    update_voice(l, now_ms);
    l->agent_xid = 0;
    l->down_open = false;
    l->popup.xid = 0;
}

void link_on_mtu(link_core_t *l, uint16_t mtu, uint32_t now_ms) {
    l->mtu = mtu;
    update_voice(l, now_ms);
}

void link_on_subscribe(link_core_t *l, bool on, uint32_t now_ms) {
    const bool was = l->subscribed;
    l->subscribed = on && l->connected;
    if (l->subscribed && !was) send_hello(l, now_ms);
    update_voice(l, now_ms);
}

static void on_hello_ack(link_core_t *l, const bp_packet_t *p, uint32_t now_ms) {
    bp_hello_ack_t ack;
    if (bp_read_hello_ack(p, &ack) != BP_OK) return;
    if (bp_check_version(ack.major, ack.minor) != BP_OK) {
        l->version_bad = true;
        send_error(l, BP_ERR_VERSION, p->type, p->xid);
        return;
    }
    l->version_bad = false;
    l->acked = true;
    l->utc = ack.utc;
    l->utc_ms = now_ms;
    send_device_state(l, true, now_ms);
    // After a reconnect, ask again for the groups that the screen lacks.
    if (l->has_turns) request_turns(l, now_ms);
}

static void on_agent_state(link_core_t *l, const bp_packet_t *p) {
    bp_agent_state_t s;
    if (bp_read_agent_state(p, &s) != BP_OK) return;
    if (!bp_xid_accept(l->agent_xid, p->xid)) {
        drop(l, p);
        return;
    }
    char text[LINK_TEXT_MAX + 1];
    size_t n = s.text.len < LINK_TEXT_MAX ? s.text.len : LINK_TEXT_MAX;
    if (n) memcpy(text, s.text.ptr, n);
    text[n] = '\0';
    if (l->io.agent_state) l->io.agent_state(l->io.ctx, s.state, s.failure, text);
}

static void on_play_start(link_core_t *l, const bp_packet_t *p) {
    bp_play_start_t s;
    if (bp_read_play_start(p, &s) != BP_OK) return;
    // Team speech is P1. The first version has no Team owner, so it ignores the stream.
    if (s.channel != BP_CHANNEL_AGENT && s.channel != BP_CHANNEL_POPUP) return;
    const bool popup = s.channel == BP_CHANNEL_POPUP;
    if (!bp_xid_accept(popup ? l->popup.xid : l->agent_xid, p->xid) || (popup && !l->popup.audio)) {
        drop(l, p);
        return;
    }
    // HELLO offers only IMA ADPCM, so any other codec value is wrong here.
    if (s.codec != BP_CODEC_ADPCM) {
        send_error(l, BP_ERR_FIELD, p->type, p->xid);
        return;
    }
    l->down_open = true;
    l->down_xid = p->xid;
    l->down_channel = s.channel;
    bp_seq_reset(&l->down_seq);
    if (popup) {
        if (l->io.popup_play_start) l->io.popup_play_start(l->io.ctx);
    } else if (l->io.play_start) {
        l->io.play_start(l->io.ctx);
    }
}

static void on_audio_down(link_core_t *l, const bp_packet_t *p) {
    if (!l->down_open || !bp_xid_accept(l->down_xid, p->xid)) {
        drop(l, p);
        return;
    }
    if (bp_check_audio_block(BP_CODEC_ADPCM, p->payload_len) != BP_OK) {
        send_error(l, BP_ERR_LENGTH, p->type, p->xid);
        return;
    }
    const uint8_t missing = bp_seq_accept(&l->down_seq, p->seq);
    if (l->io.audio_down) l->io.audio_down(l->io.ctx, p->payload, p->payload_len, missing);
}

static void on_play_end(link_core_t *l, const bp_packet_t *p) {
    if (!l->down_open || !bp_xid_accept(l->down_xid, p->xid)) {
        drop(l, p);
        return;
    }
    l->down_open = false;
    if (l->io.play_end) l->io.play_end(l->io.ctx);
}

// Doc 07: the data messages need a HELLO_ACK with the same protocol version.
static bool data_allowed(const link_core_t *l) { return l->acked && !l->version_bad; }

static void on_live_stats(link_core_t *l, const bp_packet_t *p, uint32_t now_ms) {
    bp_live_stats_t s;
    if (!data_allowed(l) || bp_read_live_stats(p, &s) != BP_OK) return;
    l->live = s;
    l->has_live = true;
    l->live_ms = now_ms;
    if (l->io.live_stats) l->io.live_stats(l->io.ctx, &l->live);
}

static void copy_text(char *dst, const bp_text_t *t) {
    if (t->len) memcpy(dst, t->ptr, t->len);
    dst[t->len] = '\0';
}

static void on_run_report(link_core_t *l, const bp_packet_t *p, uint32_t now_ms) {
    if (!data_allowed(l)) return;
    bp_run_report_t r;
    const bp_err_t err = bp_read_run_report(p, &r);
    if (err != BP_OK) {
        // The old report stays on the screen.
        send_error(l, (uint8_t)err, p->type, p->xid);
        return;
    }
    l->report.run_no = r.run_no;
    l->report.distance_m = r.distance_m;
    l->report.turns = r.turns;
    l->report.iq = r.iq;
    copy_text(l->report.problem, &r.problem);
    copy_text(l->report.tip, &r.tip);
    l->has_report = true;
    l->report_ms = now_ms;
    if (l->io.run_report) l->io.run_report(l->io.ctx, &l->report);
}

const link_turn_group_t *link_turns_group(const link_turns_t *t, uint8_t group) {
    if (!t) return NULL;
    for (int i = 0; i < LINK_TURN_SLOTS; i++) {
        if (t->slot[i].used && t->slot[i].group == group) return &t->slot[i];
    }
    return NULL;
}

// The groups that the device keeps: the focus group and its neighbors, LINK_TURN_SLOTS in all.
static void turn_window(const link_turns_t *t, uint8_t *first, uint8_t *count) {
    const uint8_t n = t->n_groups;
    uint8_t start = t->focus > 0 ? (uint8_t)(t->focus - 1) : 0;
    if (n >= LINK_TURN_SLOTS && start + LINK_TURN_SLOTS > n) start = (uint8_t)(n - LINK_TURN_SLOTS);
    if (n < LINK_TURN_SLOTS) start = 0;
    *first = start;
    *count = n < LINK_TURN_SLOTS ? n : LINK_TURN_SLOTS;
}

// Drop the groups outside the window. Ask for the missing groups in one TURN_REQUEST.
static void request_turns(link_core_t *l, uint32_t now_ms) {
    link_turns_t *t = &l->turns;
    uint8_t first, count;
    turn_window(t, &first, &count);
    for (int i = 0; i < LINK_TURN_SLOTS; i++) {
        if (t->slot[i].used && (t->slot[i].group < first || t->slot[i].group >= first + count)) t->slot[i].used = false;
    }
    int lo = -1, hi = -1;
    for (uint8_t g = first; g < first + count; g++) {
        if (link_turns_group(t, g)) continue;
        if (lo < 0) lo = g;
        hi = g;
    }
    l->turn_req_open = lo >= 0;
    if (lo < 0 || !can_send(l) || !data_allowed(l)) return;
    uint8_t pkt[BP_HEADER_SIZE + BP_TURN_REQUEST_SIZE];
    send(l, LINK_LANE_CONTROL, pkt, bp_build_turn_request(pkt, sizeof pkt, t->run_no, (uint8_t)lo, (uint8_t)(hi - lo + 1)));
    l->turn_req_ms = now_ms;
}

static void on_turn_shape(link_core_t *l, const bp_packet_t *p, uint32_t now_ms) {
    if (!data_allowed(l)) return;
    bp_turn_shape_t r;
    const bp_err_t err = bp_read_turn_shape(p, &r);
    if (err != BP_OK) {
        // The old shape stays on the screen.
        send_error(l, (uint8_t)err, p->type, p->xid);
        return;
    }
    memset(&l->turns, 0, sizeof l->turns);
    l->turns.run_no = r.run_no;
    l->turns.source = r.source;
    l->turns.n_turns = r.n_turns;
    l->turns.n_groups = (uint8_t)((r.n_turns + BP_GROUP_TURNS - 1u) / BP_GROUP_TURNS);
    l->has_turns = true;
    l->turn_req_tries = 0;
    request_turns(l, now_ms);
    if (l->io.turn_shape) l->io.turn_shape(l->io.ctx, &l->turns);
}

static void on_turn_group(link_core_t *l, const bp_packet_t *p, uint32_t now_ms) {
    if (!data_allowed(l)) return;
    bp_turn_group_t r;
    bp_err_t err = bp_read_turn_group(p, &r);
    link_turns_t *t = &l->turns;
    if (err == BP_OK && (!l->has_turns || r.run_no != t->run_no)) err = BP_ERR_STATE;
    if (err == BP_OK && r.group >= t->n_groups) err = BP_ERR_FIELD;
    if (err == BP_OK) {
        const unsigned left = t->n_turns - (unsigned)r.group * BP_GROUP_TURNS;
        if (r.n_turns != (left < BP_GROUP_TURNS ? left : BP_GROUP_TURNS)) err = BP_ERR_FIELD;
    }
    if (err != BP_OK) {
        send_error(l, (uint8_t)err, p->type, p->xid);
        return;
    }
    uint8_t first, count;
    turn_window(t, &first, &count);
    if (r.group < first || r.group >= first + count) return;   // A late answer for an old focus.
    link_turn_group_t *s = (link_turn_group_t *)link_turns_group(t, r.group);
    for (int i = 0; !s && i < LINK_TURN_SLOTS; i++) {
        if (!t->slot[i].used) s = &t->slot[i];
    }
    if (!s) return;
    s->used = true;
    s->group = r.group;
    s->n_turns = r.n_turns;
    s->n_points = r.n_points;
    memcpy(s->turns, r.turns, sizeof r.turns[0] * r.n_turns);
    memcpy(s->points, r.points, 2u * r.n_points);
    bool missing = false;
    for (uint8_t g = first; g < first + count; g++) missing |= link_turns_group(t, g) == NULL;
    l->turn_req_open = missing;
    (void)now_ms;
    if (l->io.turn_shape) l->io.turn_shape(l->io.ctx, t);
}

void link_turns_focus(link_core_t *l, uint8_t group, uint32_t now_ms) {
    if (!l->has_turns) return;
    if (l->turns.n_groups && group >= l->turns.n_groups) group = (uint8_t)(l->turns.n_groups - 1);
    l->turns.focus = group;
    l->turn_req_tries = 0;
    request_turns(l, now_ms);
}

static void on_show_page(link_core_t *l, const bp_packet_t *p) {
    if (!data_allowed(l)) return;
    uint8_t page;
    const bp_err_t err = bp_read_show_page(p, &page);
    if (err != BP_OK) {
        send_error(l, (uint8_t)err, p->type, p->xid);
        return;
    }
    if (l->io.show_page) l->io.show_page(l->io.ctx, page);
}

static void send_popup_end(link_core_t *l, uint8_t reason) {
    if (!can_send(l)) return;
    uint8_t pkt[8];
    send(l, LINK_LANE_CONTROL, pkt, bp_build_popup_end(pkt, sizeof pkt, l->popup.xid, reason));
}

// D-31: a popup stops all other audio at once. A new popup replaces the old one.
static void on_popup(link_core_t *l, const bp_packet_t *p) {
    if (!data_allowed(l)) return;
    bp_popup_t r;
    bp_err_t err = bp_read_popup(p, &r);
    if (err == BP_OK && (p->xid < 0x80 || p->xid == 0xFF)) err = BP_ERR_FIELD;
    if (err != BP_OK) {
        send_error(l, (uint8_t)err, p->type, p->xid);
        return;
    }
    close_exchange(l);
    if (l->popup.xid) {
        close_down(l, BP_CHANNEL_POPUP);
        send_popup_end(l, BP_POPUP_END_REPLACED);
    }
    l->popup.xid = p->xid;
    l->popup.kind = r.kind;
    l->popup.audio = r.audio;
    copy_text(l->popup.title, &r.title);
    copy_text(l->popup.body, &r.body);
    if (l->io.popup) l->io.popup(l->io.ctx, &l->popup);
}

void link_popup_end(link_core_t *l, uint8_t reason) {
    if (!l->popup.xid) return;
    close_down(l, BP_CHANNEL_POPUP);
    send_popup_end(l, reason);
    l->popup.xid = 0;
}

void link_on_rx(link_core_t *l, const uint8_t *buf, size_t len, uint32_t now_ms) {
    if (!l->connected) return;
    l->last_rx_ms = now_ms;
    bp_packet_t p;
    const bp_err_t err = bp_parse(buf, len, &p);
    if (err != BP_OK) {
        send_error(l, (uint8_t)err, len > 0 ? buf[0] : 0, len > 2 ? buf[2] : 0);
        update_voice(l, now_ms);
        return;
    }
    switch (p.type) {
    case BP_HELLO_ACK: on_hello_ack(l, &p, now_ms); break;
    case BP_PHONE_STATUS: {
        bp_phone_status_t s;
        if (bp_read_phone_status(&p, &s) == BP_OK && s.utc) {
            l->utc = s.utc;
            l->utc_ms = now_ms;
        }
        break;
    }
    case BP_ERROR: break;   // The phone rejected a packet. The device has no retry rule for it.
    case BP_AGENT_STATE: on_agent_state(l, &p); break;
    case BP_PLAY_START: on_play_start(l, &p); break;
    case BP_AUDIO_DOWN: on_audio_down(l, &p); break;
    case BP_PLAY_END: on_play_end(l, &p); break;
    case BP_LIVE_STATS: on_live_stats(l, &p, now_ms); break;
    case BP_RUN_REPORT: on_run_report(l, &p, now_ms); break;
    case BP_SHOW_PAGE: on_show_page(l, &p); break;
    case BP_TURN_SHAPE: on_turn_shape(l, &p, now_ms); break;
    case BP_TURN_GROUP: on_turn_group(l, &p, now_ms); break;
    case BP_POPUP: on_popup(l, &p); break;
    default:
        // A device-to-phone type that arrives from the phone.
        send_error(l, BP_ERR_STATE, p.type, p.xid);
        break;
    }
    update_voice(l, now_ms);
}

void link_tick(link_core_t *l, uint32_t now_ms) {
    update_voice(l, now_ms);
    // One more TURN_REQUEST when the groups do not arrive in time.
    if (l->turn_req_open && l->turn_req_tries < 1 && (uint32_t)(now_ms - l->turn_req_ms) >= LINK_TURN_RETRY_MS) {
        l->turn_req_tries++;
        request_turns(l, now_ms);
    }
    if (l->acked && can_send(l) && (uint32_t)(now_ms - l->status_ms) >= LINK_DEVICE_STATUS_MS) {
        send_device_state(l, false, now_ms);
    }
}

link_voice_t link_voice(const link_core_t *l) { return l->voice; }

uint8_t link_agent_start(link_core_t *l) {
    if (l->voice != LINK_VOICE_OK) return 0;
    if (l->agent_xid) close_exchange(l);
    l->agent_xid = bp_xid_next(&l->xids);
    l->up_seq = 0;
    close_down(l, BP_CHANNEL_AGENT);
    uint8_t pkt[8];
    send(l, LINK_LANE_CONTROL, pkt, bp_build_agent_start(pkt, sizeof pkt, l->agent_xid, BP_CODEC_ADPCM));
    return l->agent_xid;
}

void link_audio_up(link_core_t *l, const uint8_t block[BP_ADPCM_BLOCK]) {
    if (!l->agent_xid || l->voice != LINK_VOICE_OK) return;
    uint8_t pkt[BP_HEADER_SIZE + BP_ADPCM_BLOCK];
    send(l, LINK_LANE_AUDIO, pkt,
         bp_build_audio(pkt, sizeof pkt, BP_AUDIO_UP, l->agent_xid, l->up_seq++, block, BP_ADPCM_BLOCK));
}

void link_agent_end(link_core_t *l, uint8_t reason) {
    if (!l->agent_xid || !can_send(l)) return;
    uint8_t pkt[8];
    send(l, LINK_LANE_AUDIO, pkt, bp_build_agent_end(pkt, sizeof pkt, l->agent_xid, reason));
}

void link_agent_cancel(link_core_t *l) { close_exchange(l); }

void link_agent_done(link_core_t *l) {
    l->agent_xid = 0;
    close_down(l, BP_CHANNEL_AGENT);
}

void link_set_device(link_core_t *l, uint8_t battery, uint8_t mode, uint8_t page,
                     uint16_t free_kib, uint32_t now_ms) {
    const bool context = mode != l->mode || page != l->page;
    const bool changed = context || battery != l->battery;
    l->battery = battery;
    l->mode = mode;
    l->page = page;
    l->free_kib = free_kib;
    // A heap change alone waits for the 30-second packet.
    if (changed) send_device_state(l, context, now_ms);
}

const bp_live_stats_t *link_live(const link_core_t *l, uint32_t now_ms, bool *stale) {
    if (stale) *stale = true;
    if (!l->has_live) return NULL;
    if (stale) *stale = (uint32_t)(now_ms - l->live_ms) >= LINK_LIVE_STALE_MS;
    return &l->live;
}

const link_report_t *link_report(const link_core_t *l, uint32_t now_ms, uint32_t *age_ms) {
    if (!l->has_report) return NULL;
    if (age_ms) *age_ms = (uint32_t)(now_ms - l->report_ms);
    return &l->report;
}

const link_turns_t *link_turns(const link_core_t *l) { return l->has_turns ? &l->turns : NULL; }

const char *link_voice_name(link_voice_t voice) {
    switch (voice) {
    case LINK_VOICE_OK: return "ok";
    case LINK_VOICE_NO_LINK: return "no link";
    case LINK_VOICE_WAIT_HELLO: return "wait for HELLO_ACK";
    case LINK_VOICE_NO_ACK: return "no HELLO_ACK";
    case LINK_VOICE_VERSION: return "version mismatch";
    case LINK_VOICE_MTU: return "MTU below 172";
    case LINK_VOICE_SILENT: return "phone silent";
    default: return "?";
    }
}
