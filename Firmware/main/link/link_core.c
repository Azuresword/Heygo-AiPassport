// main/link/link_core.c
#include "link_core.h"

#include <string.h>

static void send(link_core_t *l, link_lane_t lane, const uint8_t *pkt, size_t len) {
    if (len && l->io.send) l->io.send(l->io.ctx, lane, pkt, len);
}

static bool can_send(const link_core_t *l) { return l->connected && l->subscribed; }

static void send_error(link_core_t *l, uint8_t code, uint8_t ref_type, uint8_t ref_xid) {
    if (!can_send(l)) return;
    uint8_t pkt[8];
    send(l, LINK_LANE_CONTROL, pkt, bp_build_error(pkt, sizeof pkt, code, ref_type, ref_xid));
}

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
    l->down_open = false;
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
}

static void on_agent_state(link_core_t *l, const bp_packet_t *p) {
    bp_agent_state_t s;
    if (bp_read_agent_state(p, &s) != BP_OK || !bp_xid_accept(l->agent_xid, p->xid)) return;
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
    if (s.channel != BP_CHANNEL_AGENT) return;
    if (!bp_xid_accept(l->agent_xid, p->xid)) return;
    // HELLO offers only IMA ADPCM, so any other codec value is wrong here.
    if (s.codec != BP_CODEC_ADPCM) {
        send_error(l, BP_ERR_FIELD, p->type, p->xid);
        return;
    }
    l->down_open = true;
    bp_seq_reset(&l->down_seq);
    if (l->io.play_start) l->io.play_start(l->io.ctx);
}

static void on_audio_down(link_core_t *l, const bp_packet_t *p) {
    if (!l->down_open || !bp_xid_accept(l->agent_xid, p->xid)) return;
    if (bp_check_audio_block(BP_CODEC_ADPCM, p->payload_len) != BP_OK) {
        send_error(l, BP_ERR_LENGTH, p->type, p->xid);
        return;
    }
    const uint8_t missing = bp_seq_accept(&l->down_seq, p->seq);
    if (l->io.audio_down) l->io.audio_down(l->io.ctx, p->payload, p->payload_len, missing);
}

static void on_play_end(link_core_t *l, const bp_packet_t *p) {
    if (!l->down_open || !bp_xid_accept(l->agent_xid, p->xid)) return;
    l->down_open = false;
    if (l->io.play_end) l->io.play_end(l->io.ctx);
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
    default:
        // A device-to-phone type that arrives from the phone.
        send_error(l, BP_ERR_STATE, p.type, p.xid);
        break;
    }
    update_voice(l, now_ms);
}

void link_tick(link_core_t *l, uint32_t now_ms) {
    update_voice(l, now_ms);
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
    l->down_open = false;
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
    l->down_open = false;
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
