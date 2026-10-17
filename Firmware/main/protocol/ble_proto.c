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
    if (p->payload[0] > BP_CHANNEL_TEAM || !codec_known(p->payload[1])) return BP_ERR_FIELD;
    out->channel = p->payload[0];
    out->codec = p->payload[1];
    out->speaker = text_from(p, 2);
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
    if (channel > BP_CHANNEL_TEAM || !codec_known(codec)) return 0;
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
    default: return "?";
    }
}
