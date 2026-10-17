// main/protocol/ble_proto.c
#include "ble_proto.h"

#include <string.h>

static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static uint16_t get_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Minimum payload length of each P0 type. -1 marks a type that P0 does not build.
static int min_payload(uint8_t type) {
    switch (type) {
    case BP_HELLO:         return 5;
    case BP_HELLO_ACK:     return 6;
    case BP_PHONE_STATUS:  return 7;
    case BP_DEVICE_STATUS: return 5;
    case BP_ERROR:         return 3;
    case BP_CONTEXT:       return 2;
    case BP_AGENT_START:   return 2;
    case BP_AGENT_END:     return 1;
    case BP_AGENT_CANCEL:  return 0;
    case BP_AGENT_STATE:   return 2;
    case BP_AUDIO_UP:      return 1;
    case BP_AUDIO_DOWN:    return 1;
    case BP_PLAY_START:    return 2;
    case BP_PLAY_END:      return 0;
    case BP_LIVE_STATS:    return BP_LIVE_STATS_SIZE;
    case BP_RUN_REPORT:    return BP_RUN_REPORT_FIXED;
    case BP_SHOW_PAGE:     return 1;
    case BP_TURN_SHAPE:    return BP_TURN_SHAPE_SIZE;
    case BP_TURN_GROUP:    return BP_TURN_GROUP_FIXED;
    case BP_TURN_REQUEST:  return BP_TURN_REQUEST_SIZE;
    case BP_POPUP:         return BP_POPUP_FIXED;
    case BP_POPUP_END:     return 1;
    case BP_RUN_START:     return 1;
    default:               return -1;
    }
}

bp_err_t bp_parse(const uint8_t *buf, size_t len, bp_packet_t *out) {
    if (!buf || !out || len < BP_HEADER_SIZE) return BP_ERR_LENGTH;
    int min = min_payload(buf[0]);
    if (min < 0) return BP_ERR_UNKNOWN_TYPE;
    if (len - BP_HEADER_SIZE < (size_t)min) return BP_ERR_LENGTH;
    out->type = buf[0];
    out->flags = buf[1];
    out->xid = buf[2];
    out->seq = buf[3];
    out->payload = buf + BP_HEADER_SIZE;
    out->payload_len = len - BP_HEADER_SIZE;
    return BP_OK;
}

// Each reader checks the type and the length again, so a caller that skips
// bp_parse() still cannot read past the payload.
static bp_err_t expect(const bp_packet_t *p, uint8_t type) {
    if (!p || !p->payload || p->type != type) return BP_ERR_STATE;
    if (p->payload_len < (size_t)min_payload(type)) return BP_ERR_LENGTH;
    return BP_OK;
}

static bp_text_t text_from(const bp_packet_t *p, size_t offset) {
    bp_text_t t = { p->payload + offset, p->payload_len - offset };
    return t;
}

static bool codec_known(uint8_t codec) { return codec == BP_CODEC_ADPCM || codec == BP_CODEC_PCM; }

bp_err_t bp_read_hello(const bp_packet_t *p, bp_hello_t *out) {
    bp_err_t e = expect(p, BP_HELLO);
    if (e) return e;
    out->major = p->payload[0];
    out->minor = p->payload[1];
    out->codec_mask = p->payload[2];
    out->mtu = get_u16(p->payload + 3);
    out->fw = text_from(p, 5);
    return BP_OK;
}

bp_err_t bp_read_hello_ack(const bp_packet_t *p, bp_hello_ack_t *out) {
    bp_err_t e = expect(p, BP_HELLO_ACK);
    if (e) return e;
    out->major = p->payload[0];
    out->minor = p->payload[1];
    out->utc = get_u32(p->payload + 2);
    out->app = text_from(p, 6);
    return BP_OK;
}

static bool flag_ok(uint8_t v) { return v == 0 || v == 1; }

bp_err_t bp_read_phone_status(const bp_packet_t *p, bp_phone_status_t *out) {
    bp_err_t e = expect(p, BP_PHONE_STATUS);
    if (e) return e;
    const uint8_t *b = p->payload;
    if (!flag_ok(b[4]) || !flag_ok(b[5]) || !flag_ok(b[6])) return BP_ERR_FIELD;
    out->utc = get_u32(b);
    out->online = b[4];
    out->gateway = b[5];
    out->background = b[6];
    return BP_OK;
}

bp_err_t bp_read_device_status(const bp_packet_t *p, bp_device_status_t *out) {
    bp_err_t e = expect(p, BP_DEVICE_STATUS);
    if (e) return e;
    const uint8_t *b = p->payload;
    if (b[0] > 100 && b[0] != BP_BATTERY_UNKNOWN) return BP_ERR_FIELD;
    if (b[1] > BP_MODE_REST) return BP_ERR_FIELD;
    out->battery = b[0];
    out->mode = b[1];
    out->page = b[2];            // An unknown page stays allowed.
    out->free_kib = get_u16(b + 3);
    return BP_OK;
}

bp_err_t bp_read_error(const bp_packet_t *p, bp_error_t *out) {
    bp_err_t e = expect(p, BP_ERROR);
    if (e) return e;
    out->code = p->payload[0];
    out->ref_type = p->payload[1];
    out->ref_xid = p->payload[2];
    return BP_OK;
}

bp_err_t bp_read_context(const bp_packet_t *p, bp_context_t *out) {
    bp_err_t e = expect(p, BP_CONTEXT);
    if (e) return e;
    if (p->payload[0] > BP_MODE_REST) return BP_ERR_FIELD;
    out->mode = p->payload[0];
    out->page = p->payload[1];   // An unknown page stays allowed.
    return BP_OK;
}

bp_err_t bp_read_agent_start(const bp_packet_t *p, bp_agent_start_t *out) {
    bp_err_t e = expect(p, BP_AGENT_START);
    if (e) return e;
    if (!codec_known(p->payload[0]) || p->payload[1] != BP_SAMPLE_RATE_KHZ) return BP_ERR_FIELD;
    out->codec = p->payload[0];
    out->rate_khz = p->payload[1];
    return BP_OK;
}

bp_err_t bp_read_agent_end(const bp_packet_t *p, uint8_t *reason) {
    bp_err_t e = expect(p, BP_AGENT_END);
    if (e) return e;
    if (p->payload[0] > BP_END_LIMIT) return BP_ERR_FIELD;
    *reason = p->payload[0];
    return BP_OK;
}

bp_err_t bp_read_agent_state(const bp_packet_t *p, bp_agent_state_t *out) {
    bp_err_t e = expect(p, BP_AGENT_STATE);
    if (e) return e;
    uint8_t state = p->payload[0], failure = p->payload[1];
    if (state < BP_AGENT_ACCEPTED || state > BP_AGENT_FAILED) return BP_ERR_FIELD;
    if (state == BP_AGENT_FAILED) {
        if (failure < BP_FAIL_RECOGNITION || failure > BP_FAIL_GATEWAY) return BP_ERR_FIELD;
    } else if (failure != BP_FAIL_NONE) {
        return BP_ERR_FIELD;
    }
    out->state = state;
    out->failure = failure;
    out->text = text_from(p, 2);
    return BP_OK;
}

bp_err_t bp_read_play_start(const bp_packet_t *p, bp_play_start_t *out) {
    bp_err_t e = expect(p, BP_PLAY_START);
    if (e) return e;
    if (p->payload[0] > BP_CHANNEL_POPUP || !codec_known(p->payload[1])) return BP_ERR_FIELD;
    out->channel = p->payload[0];
    out->codec = p->payload[1];
    out->speaker = text_from(p, 2);
    return BP_OK;
}

bool bp_utf8_valid(const uint8_t *p, size_t len) {
    size_t i = 0;
    while (i < len) {
        const uint8_t c = p[i];
        size_t n;
        uint32_t cp;
        if (c < 0x80) { i++; continue; }
        if ((c & 0xE0) == 0xC0) { n = 1; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { n = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { n = 3; cp = c & 0x07; }
        else return false;
        if (len - i < n + 1) return false;    // The character runs past the end.
        for (size_t k = 1; k <= n; k++) {
            if ((p[i + k] & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (p[i + k] & 0x3F);
        }
        // Reject overlong forms, surrogates, and values above U+10FFFF.
        if ((n == 1 && cp < 0x80) || (n == 2 && cp < 0x800) || (n == 3 && cp < 0x10000)) return false;
        if ((cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) return false;
        i += n + 1;
    }
    return true;
}

bp_err_t bp_read_live_stats(const bp_packet_t *p, bp_live_stats_t *out) {
    bp_err_t e = expect(p, BP_LIVE_STATS);
    if (e) return e;
    const uint8_t *b = p->payload;
    // Doc 07: an unknown run state counts as unknown, with no ERROR.
    out->run_state = b[0] <= BP_RUN_ENDED ? b[0] : BP_RUN_UNKNOWN;
    out->run_no = b[1];
    out->speed_dkmh = get_u16(b + 2);
    out->drop_m = get_u16(b + 4);
    out->distance_m = get_u16(b + 6);
    out->turns = get_u16(b + 8);
    out->time_s = get_u16(b + 10);
    return BP_OK;
}

bp_err_t bp_read_run_report(const bp_packet_t *p, bp_run_report_t *out) {
    bp_err_t e = expect(p, BP_RUN_REPORT);
    if (e) return e;
    const uint8_t *b = p->payload;
    const size_t problem_len = b[6];
    if (problem_len > p->payload_len - BP_RUN_REPORT_FIXED) return BP_ERR_LENGTH;
    const size_t tip_len = p->payload_len - BP_RUN_REPORT_FIXED - problem_len;
    if (problem_len > BP_PROBLEM_MAX || tip_len > BP_TIP_MAX) return BP_ERR_TOO_LARGE;
    if (b[5] > BP_IQ_MAX && b[5] != BP_U8_UNKNOWN) return BP_ERR_FIELD;
    const uint8_t *problem = b + BP_RUN_REPORT_FIXED;
    const uint8_t *tip = problem + problem_len;
    if (!bp_utf8_valid(problem, problem_len) || !bp_utf8_valid(tip, tip_len)) return BP_ERR_FIELD;
    out->run_no = b[0];
    out->distance_m = get_u16(b + 1);
    out->turns = get_u16(b + 3);
    out->iq = b[5];
    out->problem.ptr = problem; out->problem.len = problem_len;
    out->tip.ptr = tip;         out->tip.len = tip_len;
    return BP_OK;
}

bp_err_t bp_read_show_page(const bp_packet_t *p, uint8_t *page) {
    bp_err_t e = expect(p, BP_SHOW_PAGE);
    if (e) return e;
    if (p->payload[0] != BP_PAGE_REPORT) return BP_ERR_FIELD;
    *page = p->payload[0];
    return BP_OK;
}

static size_t header(uint8_t *out, size_t cap, size_t payload, uint8_t type, uint8_t xid, uint8_t seq) {
    if (!out || cap < BP_HEADER_SIZE + payload) return 0;
    out[0] = type; out[1] = 0; out[2] = xid; out[3] = seq;
    return BP_HEADER_SIZE + payload;
}

static size_t text_len(const char *s) { return s ? strlen(s) : 0; }

size_t bp_build_hello(uint8_t *out, size_t cap, uint8_t codec_mask, uint16_t mtu, const char *fw) {
    size_t tl = text_len(fw);
    if (tl > BP_VERSION_TEXT_MAX) return 0;
    size_t n = header(out, cap, 5 + tl, BP_HELLO, 0, 0);
    if (!n) return 0;
    uint8_t *b = out + BP_HEADER_SIZE;
    b[0] = BP_PROTO_MAJOR; b[1] = BP_PROTO_MINOR; b[2] = codec_mask;
    put_u16(b + 3, mtu);
    if (tl) memcpy(b + 5, fw, tl);
    return n;
}

size_t bp_build_hello_ack(uint8_t *out, size_t cap, uint8_t major, uint8_t minor, uint32_t utc, const char *app) {
    size_t tl = text_len(app);
    if (tl > BP_VERSION_TEXT_MAX) return 0;
    size_t n = header(out, cap, 6 + tl, BP_HELLO_ACK, 0, 0);
    if (!n) return 0;
    uint8_t *b = out + BP_HEADER_SIZE;
    b[0] = major; b[1] = minor;
    put_u32(b + 2, utc);
    if (tl) memcpy(b + 6, app, tl);
    return n;
}

size_t bp_build_phone_status(uint8_t *out, size_t cap, uint32_t utc, bool online, bool gateway, bool background) {
    size_t n = header(out, cap, 7, BP_PHONE_STATUS, 0, 0);
    if (!n) return 0;
    uint8_t *b = out + BP_HEADER_SIZE;
    put_u32(b, utc);
    b[4] = online; b[5] = gateway; b[6] = background;
    return n;
}

size_t bp_build_device_status(uint8_t *out, size_t cap, uint8_t battery, uint8_t mode, uint8_t page, uint16_t free_kib) {
    if ((battery > 100 && battery != BP_BATTERY_UNKNOWN) || mode > BP_MODE_REST) return 0;
    size_t n = header(out, cap, 5, BP_DEVICE_STATUS, 0, 0);
    if (!n) return 0;
    uint8_t *b = out + BP_HEADER_SIZE;
    b[0] = battery; b[1] = mode; b[2] = page;
    put_u16(b + 3, free_kib);
    return n;
}

size_t bp_build_error(uint8_t *out, size_t cap, uint8_t code, uint8_t ref_type, uint8_t ref_xid) {
    size_t n = header(out, cap, 3, BP_ERROR, 0, 0);
    if (!n) return 0;
    out[4] = code; out[5] = ref_type; out[6] = ref_xid;
    return n;
}

size_t bp_build_context(uint8_t *out, size_t cap, uint8_t mode, uint8_t page) {
    if (mode > BP_MODE_REST) return 0;
    size_t n = header(out, cap, 2, BP_CONTEXT, 0, 0);
    if (!n) return 0;
    out[4] = mode; out[5] = page;
    return n;
}

size_t bp_build_agent_start(uint8_t *out, size_t cap, uint8_t xid, uint8_t codec) {
    if (!codec_known(codec)) return 0;
    size_t n = header(out, cap, 2, BP_AGENT_START, xid, 0);
    if (!n) return 0;
    out[4] = codec; out[5] = BP_SAMPLE_RATE_KHZ;
    return n;
}

size_t bp_build_agent_end(uint8_t *out, size_t cap, uint8_t xid, uint8_t reason) {
    if (reason > BP_END_LIMIT) return 0;
    size_t n = header(out, cap, 1, BP_AGENT_END, xid, 0);
    if (!n) return 0;
    out[4] = reason;
    return n;
}

size_t bp_build_agent_cancel(uint8_t *out, size_t cap, uint8_t xid) {
    return header(out, cap, 0, BP_AGENT_CANCEL, xid, 0);
}

size_t bp_build_agent_state(uint8_t *out, size_t cap, uint8_t xid, uint8_t state, uint8_t failure, const char *text) {
    size_t tl = text_len(text);
    size_t n = header(out, cap, 2 + tl, BP_AGENT_STATE, xid, 0);
    if (!n) return 0;
    out[4] = state; out[5] = failure;
    if (tl) memcpy(out + 6, text, tl);
    return n;
}

size_t bp_build_audio(uint8_t *out, size_t cap, uint8_t type, uint8_t xid, uint8_t seq,
                      const uint8_t *block, size_t block_len) {
    if ((type != BP_AUDIO_UP && type != BP_AUDIO_DOWN) || !block || block_len == 0) return 0;
    size_t n = header(out, cap, block_len, type, xid, seq);
    if (!n) return 0;
    memcpy(out + BP_HEADER_SIZE, block, block_len);
    return n;
}

size_t bp_build_play_start(uint8_t *out, size_t cap, uint8_t xid, uint8_t channel, uint8_t codec, const char *speaker) {
    if (channel > BP_CHANNEL_POPUP || !codec_known(codec)) return 0;
    size_t tl = text_len(speaker);
    size_t n = header(out, cap, 2 + tl, BP_PLAY_START, xid, 0);
    if (!n) return 0;
    out[4] = channel; out[5] = codec;
    if (tl) memcpy(out + 6, speaker, tl);
    return n;
}

size_t bp_build_play_end(uint8_t *out, size_t cap, uint8_t xid) {
    return header(out, cap, 0, BP_PLAY_END, xid, 0);
}

size_t bp_build_live_stats(uint8_t *out, size_t cap, const bp_live_stats_t *s) {
    if (!s || s->run_state > BP_RUN_ENDED) return 0;
    size_t n = header(out, cap, BP_LIVE_STATS_SIZE, BP_LIVE_STATS, 0, 0);
    if (!n) return 0;
    uint8_t *b = out + BP_HEADER_SIZE;
    b[0] = s->run_state; b[1] = s->run_no;
    put_u16(b + 2, s->speed_dkmh);
    put_u16(b + 4, s->drop_m);
    put_u16(b + 6, s->distance_m);
    put_u16(b + 8, s->turns);
    put_u16(b + 10, s->time_s);
    return n;
}

size_t bp_build_run_report(uint8_t *out, size_t cap, uint8_t run_no, uint16_t distance_m, uint16_t turns,
                           uint8_t iq, const char *problem, const char *tip) {
    const size_t pl = text_len(problem), tl = text_len(tip);
    if (pl > BP_PROBLEM_MAX || tl > BP_TIP_MAX || (iq > BP_IQ_MAX && iq != BP_U8_UNKNOWN)) return 0;
    size_t n = header(out, cap, BP_RUN_REPORT_FIXED + pl + tl, BP_RUN_REPORT, 0, 0);
    if (!n) return 0;
    uint8_t *b = out + BP_HEADER_SIZE;
    b[0] = run_no;
    put_u16(b + 1, distance_m);
    put_u16(b + 3, turns);
    b[5] = iq;
    b[6] = (uint8_t)pl;
    if (pl) memcpy(b + BP_RUN_REPORT_FIXED, problem, pl);
    if (tl) memcpy(b + BP_RUN_REPORT_FIXED + pl, tip, tl);
    return n;
}

size_t bp_build_show_page(uint8_t *out, size_t cap, uint8_t page) {
    if (page != BP_PAGE_REPORT) return 0;
    size_t n = header(out, cap, 1, BP_SHOW_PAGE, 0, 0);
    if (!n) return 0;
    out[4] = page;
    return n;
}

static bp_err_t check_turns(uint8_t n_turns, const bp_turn_t *turns, uint8_t n_points) {
    for (uint8_t i = 0; i < n_turns; i++) {
        const bp_turn_t *t = &turns[i];
        if (t->first >= t->last || t->last >= n_points) return BP_ERR_FIELD;
        if (t->side > BP_SIDE_HEEL && t->side != BP_U8_UNKNOWN) return BP_ERR_FIELD;
        if (t->edge_deg > BP_EDGE_MAX && t->edge_deg != BP_U8_UNKNOWN) return BP_ERR_FIELD;
    }
    return BP_OK;
}

static bp_err_t check_group(uint8_t n_turns, const bp_turn_t *turns, uint8_t n_points) {
    if (n_turns > BP_GROUP_TURNS || n_points > BP_GROUP_POINTS_MAX) return BP_ERR_TOO_LARGE;
    if (n_turns == 0 || n_points < BP_GROUP_POINTS_MIN) return BP_ERR_FIELD;
    return check_turns(n_turns, turns, n_points);
}

bp_err_t bp_read_turn_shape(const bp_packet_t *p, bp_turn_shape_t *out) {
    bp_err_t e = expect(p, BP_TURN_SHAPE);
    if (e) return e;
    out->run_no = p->payload[0];
    out->source = p->payload[1];
    out->n_turns = p->payload[2];
    return out->source > BP_TURN_SRC_SCHEMATIC ? BP_ERR_FIELD : BP_OK;
}

size_t bp_build_turn_shape(uint8_t *out, size_t cap, uint8_t run_no, uint8_t source, uint8_t n_turns) {
    if (source > BP_TURN_SRC_SCHEMATIC) return 0;
    size_t n = header(out, cap, BP_TURN_SHAPE_SIZE, BP_TURN_SHAPE, 0, 0);
    if (!n) return 0;
    out[4] = run_no; out[5] = source; out[6] = n_turns;
    return n;
}

bp_err_t bp_read_turn_group(const bp_packet_t *p, bp_turn_group_t *out) {
    bp_err_t e = expect(p, BP_TURN_GROUP);
    if (e) return e;
    const uint8_t *b = p->payload;
    const uint8_t k = b[2], n = b[3];
    if (k > BP_GROUP_TURNS || n > BP_GROUP_POINTS_MAX) return BP_ERR_TOO_LARGE;
    if (p->payload_len != (size_t)BP_TURN_GROUP_FIXED + (size_t)BP_TURN_ENTRY * k + 2u * n) return BP_ERR_LENGTH;
    out->run_no = b[0];
    out->group = b[1];
    out->n_turns = k;
    out->n_points = n;
    const uint8_t *t = b + BP_TURN_GROUP_FIXED;
    for (uint8_t i = 0; i < k; i++, t += BP_TURN_ENTRY) {
        out->turns[i] = (bp_turn_t){ t[0], t[1], t[2], t[3], t[4], t[5] };
    }
    out->points = t;
    return check_group(k, out->turns, n);
}

size_t bp_build_turn_group(uint8_t *out, size_t cap, uint8_t run_no, uint8_t group, uint8_t n_turns,
                           const bp_turn_t *turns, uint8_t n_points, const uint8_t *points) {
    if (!turns || !points || check_group(n_turns, turns, n_points) != BP_OK) return 0;
    const size_t len = BP_TURN_GROUP_FIXED + (size_t)BP_TURN_ENTRY * n_turns + 2u * n_points;
    size_t n = header(out, cap, len, BP_TURN_GROUP, 0, 0);
    if (!n) return 0;
    uint8_t *b = out + 4;
    b[0] = run_no; b[1] = group; b[2] = n_turns; b[3] = n_points;
    uint8_t *t = b + BP_TURN_GROUP_FIXED;
    for (uint8_t i = 0; i < n_turns; i++, t += BP_TURN_ENTRY) {
        t[0] = turns[i].first; t[1] = turns[i].last; t[2] = turns[i].side;
        t[3] = turns[i].speed_kmh; t[4] = turns[i].edge_deg; t[5] = turns[i].radius_m;
    }
    memcpy(t, points, 2u * n_points);
    return n;
}

bp_err_t bp_read_turn_request(const bp_packet_t *p, bp_turn_request_t *out) {
    bp_err_t e = expect(p, BP_TURN_REQUEST);
    if (e) return e;
    out->run_no = p->payload[0];
    out->first_group = p->payload[1];
    out->count = p->payload[2];
    return (out->count == 0 || out->count > BP_REQUEST_GROUPS_MAX) ? BP_ERR_FIELD : BP_OK;
}

size_t bp_build_turn_request(uint8_t *out, size_t cap, uint8_t run_no, uint8_t first_group, uint8_t count) {
    if (count == 0 || count > BP_REQUEST_GROUPS_MAX) return 0;
    size_t n = header(out, cap, BP_TURN_REQUEST_SIZE, BP_TURN_REQUEST, 0, 0);
    if (!n) return 0;
    out[4] = run_no; out[5] = first_group; out[6] = count;
    return n;
}

bp_err_t bp_read_popup(const bp_packet_t *p, bp_popup_t *out) {
    bp_err_t e = expect(p, BP_POPUP);
    if (e) return e;
    const uint8_t *b = p->payload;
    if (b[0] < BP_POPUP_FALL || b[0] > BP_POPUP_BADGE || b[1] > 1) return BP_ERR_FIELD;
    const size_t title_len = b[2];
    if (title_len > p->payload_len - BP_POPUP_FIXED) return BP_ERR_LENGTH;
    const size_t body_len = p->payload_len - BP_POPUP_FIXED - title_len;
    if (title_len > BP_POPUP_TITLE_MAX || body_len > BP_POPUP_BODY_MAX) return BP_ERR_TOO_LARGE;
    const uint8_t *title = b + BP_POPUP_FIXED, *body = title + title_len;
    if (!bp_utf8_valid(title, title_len) || !bp_utf8_valid(body, body_len)) return BP_ERR_FIELD;
    out->kind = b[0];
    out->audio = b[1];
    out->title.ptr = title; out->title.len = title_len;
    out->body.ptr = body;   out->body.len = body_len;
    return BP_OK;
}

bp_err_t bp_read_popup_end(const bp_packet_t *p, uint8_t *reason) {
    bp_err_t e = expect(p, BP_POPUP_END);
    if (e) return e;
    if (p->payload[0] > BP_POPUP_END_REPLACED) return BP_ERR_FIELD;
    *reason = p->payload[0];
    return BP_OK;
}

size_t bp_build_popup(uint8_t *out, size_t cap, uint8_t xid, uint8_t kind, bool audio, const char *title, const char *body) {
    const size_t tl = text_len(title), bl = text_len(body);
    if (kind < BP_POPUP_FALL || kind > BP_POPUP_BADGE || tl > BP_POPUP_TITLE_MAX || bl > BP_POPUP_BODY_MAX) return 0;
    size_t n = header(out, cap, BP_POPUP_FIXED + tl + bl, BP_POPUP, xid, 0);
    if (!n) return 0;
    uint8_t *b = out + BP_HEADER_SIZE;
    b[0] = kind; b[1] = audio ? 1 : 0; b[2] = (uint8_t)tl;
    if (tl) memcpy(b + BP_POPUP_FIXED, title, tl);
    if (bl) memcpy(b + BP_POPUP_FIXED + tl, body, bl);
    return n;
}

size_t bp_build_popup_end(uint8_t *out, size_t cap, uint8_t xid, uint8_t reason) {
    if (reason > BP_POPUP_END_REPLACED) return 0;
    size_t n = header(out, cap, 1, BP_POPUP_END, xid, 0);
    if (!n) return 0;
    out[4] = reason;
    return n;
}

bp_err_t bp_read_run_start(const bp_packet_t *p, uint8_t *run_no) {
    bp_err_t e = expect(p, BP_RUN_START);
    if (e) return e;
    if (p->payload[0] == 0 || p->payload[0] == BP_U8_UNKNOWN) return BP_ERR_FIELD;
    *run_no = p->payload[0];
    return BP_OK;
}

size_t bp_build_run_start(uint8_t *out, size_t cap, uint8_t run_no) {
    if (run_no == 0 || run_no == BP_U8_UNKNOWN) return 0;
    size_t n = header(out, cap, 1, BP_RUN_START, 0, 0);
    if (!n) return 0;
    out[4] = run_no;
    return n;
}

bp_err_t bp_check_version(uint8_t major, uint8_t minor) {
    return (major == BP_PROTO_MAJOR && minor == BP_PROTO_MINOR) ? BP_OK : BP_ERR_VERSION;
}

size_t bp_audio_block_size(uint8_t codec) {
    if (codec == BP_CODEC_ADPCM) return BP_ADPCM_BLOCK;
    if (codec == BP_CODEC_PCM) return BP_PCM_BLOCK;
    return 0;
}

bp_err_t bp_check_audio_block(uint8_t codec, size_t len) {
    size_t want = bp_audio_block_size(codec);
    if (!want) return BP_ERR_FIELD;
    return len == want ? BP_OK : BP_ERR_LENGTH;
}

void bp_xid_init_device(bp_xid_gen_t *g) { g->lo = 0x01; g->hi = 0x7F; g->next = 0x01; }
void bp_xid_init_phone(bp_xid_gen_t *g)  { g->lo = 0x80; g->hi = 0xFE; g->next = 0x80; }

uint8_t bp_xid_next(bp_xid_gen_t *g) {
    uint8_t v = g->next;
    g->next = (g->next == g->hi) ? g->lo : (uint8_t)(g->next + 1);
    return v;
}

bool bp_xid_accept(uint8_t current, uint8_t xid) { return current != 0 && xid == current; }

void bp_seq_reset(bp_seq_t *s) { s->expect = 0; s->started = false; }

uint8_t bp_seq_accept(bp_seq_t *s, uint8_t seq) {
    uint8_t missing = (uint8_t)(seq - s->expect);   // Wraps after 255 by design.
    s->expect = (uint8_t)(seq + 1);
    s->started = true;
    return missing;
}

const char *bp_type_name(uint8_t type) {
    switch (type) {
    case BP_HELLO: return "HELLO";
    case BP_HELLO_ACK: return "HELLO_ACK";
    case BP_PHONE_STATUS: return "PHONE_STATUS";
    case BP_DEVICE_STATUS: return "DEVICE_STATUS";
    case BP_ERROR: return "ERROR";
    case BP_CONTEXT: return "CONTEXT";
    case BP_AGENT_START: return "AGENT_START";
    case BP_AGENT_END: return "AGENT_END";
    case BP_AGENT_CANCEL: return "AGENT_CANCEL";
    case BP_AGENT_STATE: return "AGENT_STATE";
    case BP_AUDIO_UP: return "AUDIO_UP";
    case BP_AUDIO_DOWN: return "AUDIO_DOWN";
    case BP_PLAY_START: return "PLAY_START";
    case BP_PLAY_END: return "PLAY_END";
    case BP_LIVE_STATS: return "LIVE_STATS";
    case BP_RUN_REPORT: return "RUN_REPORT";
    case BP_SHOW_PAGE: return "SHOW_PAGE";
    case BP_TURN_SHAPE: return "TURN_SHAPE";
    case BP_TURN_GROUP: return "TURN_GROUP";
    case BP_TURN_REQUEST: return "TURN_REQUEST";
    case BP_POPUP: return "POPUP";
    case BP_POPUP_END: return "POPUP_END";
    case BP_RUN_START: return "RUN_START";
    default: return "?";
    }
}
