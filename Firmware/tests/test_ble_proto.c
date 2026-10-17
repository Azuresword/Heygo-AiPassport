// Host tests for the packet layer in Sprint 0, task T-2.
// Every value comes from doc 07, protocol version 0.2.
#include "protocol/ble_proto.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define CAP (BP_MTU_MIN - 3)   // 169 bytes: the ATT payload at the minimum MTU

static uint8_t buf[256];

static void roundtrip_link(void) {
    bp_packet_t p;
    size_t n = bp_build_hello(buf, CAP, BP_CODEC_MASK_ADPCM | BP_CODEC_MASK_PCM, 247, "0.1.0");
    assert(n == 4 + 5 + 5);
    assert(buf[4] == 0 && buf[5] == 2);                // protocol 0.2
    assert(buf[7] == 0xF7 && buf[8] == 0x00);          // MTU 247, little-endian
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_hello_t h; assert(bp_read_hello(&p, &h) == BP_OK);
    assert(h.major == 0 && h.minor == 2 && h.mtu == 247 && h.codec_mask == 3);
    assert(h.fw.len == 5 && memcmp(h.fw.ptr, "0.1.0", 5) == 0);

    n = bp_build_hello_ack(buf, CAP, 0, 1, 1760000000u, "app-1.0");
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_hello_ack_t a; assert(bp_read_hello_ack(&p, &a) == BP_OK);
    assert(a.major == 0 && a.minor == 1 && a.utc == 1760000000u && a.app.len == 7);

    n = bp_build_phone_status(buf, CAP, 42u, true, false, true);
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_phone_status_t ps; assert(bp_read_phone_status(&p, &ps) == BP_OK);
    assert(ps.utc == 42u && ps.online && !ps.gateway && ps.background);

    n = bp_build_device_status(buf, CAP, BP_BATTERY_UNKNOWN, BP_MODE_REST, BP_PAGE_COACH, 180);
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_device_status_t ds; assert(bp_read_device_status(&p, &ds) == BP_OK);
    assert(ds.battery == 0xFF && ds.mode == 1 && ds.page == 1 && ds.free_kib == 180);

    n = bp_build_error(buf, CAP, BP_ERR_FIELD, BP_AGENT_START, 5);
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_error_t er; assert(bp_read_error(&p, &er) == BP_OK);
    assert(er.code == 7 && er.ref_type == 0x20 && er.ref_xid == 5);
}

static void roundtrip_agent_and_audio(void) {
    bp_packet_t p;
    size_t n = bp_build_context(buf, CAP, BP_MODE_REST, BP_PAGE_COACH);
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_context_t c; assert(bp_read_context(&p, &c) == BP_OK && c.mode == 1 && c.page == 1);

    n = bp_build_agent_start(buf, CAP, 5, BP_CODEC_ADPCM);
    assert(bp_parse(buf, n, &p) == BP_OK && p.xid == 5 && p.seq == 0);
    bp_agent_start_t s; assert(bp_read_agent_start(&p, &s) == BP_OK && s.codec == 0 && s.rate_khz == 16);

    n = bp_build_agent_end(buf, CAP, 5, BP_END_LIMIT);
    uint8_t reason; assert(bp_parse(buf, n, &p) == BP_OK && bp_read_agent_end(&p, &reason) == BP_OK && reason == 1);

    n = bp_build_agent_cancel(buf, CAP, 5);
    assert(n == 4 && bp_parse(buf, n, &p) == BP_OK && p.type == BP_AGENT_CANCEL);

    // The longest AGENT_STATE text at MTU 172 is 163 bytes.
    char text[200]; memset(text, 'a', sizeof text);
    text[163] = '\0';
    n = bp_build_agent_state(buf, CAP, 5, BP_AGENT_ANSWERING, BP_FAIL_NONE, text);
    assert(n == CAP);
    text[163] = 'a'; text[164] = '\0';
    assert(bp_build_agent_state(buf, CAP, 5, BP_AGENT_ANSWERING, BP_FAIL_NONE, text) == 0);

    n = bp_build_agent_state(buf, CAP, 5, BP_AGENT_FAILED, BP_FAIL_RECOGNITION, "");
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_agent_state_t st; assert(bp_read_agent_state(&p, &st) == BP_OK && st.state == 6 && st.failure == 1);

    // One ADPCM packet uses 168 bytes, and fits at MTU 172.
    uint8_t block[BP_ADPCM_BLOCK] = {0};
    n = bp_build_audio(buf, CAP, BP_AUDIO_UP, 5, 9, block, sizeof block);
    assert(n == 168);
    assert(bp_parse(buf, n, &p) == BP_OK && p.seq == 9);
    assert(bp_check_audio_block(BP_CODEC_ADPCM, p.payload_len) == BP_OK);

    n = bp_build_play_start(buf, CAP, 5, BP_CHANNEL_AGENT, BP_CODEC_ADPCM, "");
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_play_start_t ps; assert(bp_read_play_start(&p, &ps) == BP_OK && ps.channel == 0 && ps.codec == 0);

    n = bp_build_play_end(buf, CAP, 5);
    assert(n == 4 && bp_parse(buf, n, &p) == BP_OK && p.type == BP_PLAY_END);
}

static void error_codes(void) {
    bp_packet_t p;
    // Code 2: a header shorter than 4 bytes, or a payload below the minimum.
    assert(bp_parse(buf, 3, &p) == BP_ERR_LENGTH);
    size_t n = bp_build_context(buf, CAP, 0, 0);
    assert(bp_parse(buf, n - 1, &p) == BP_ERR_LENGTH);
    n = bp_build_phone_status(buf, CAP, 0, false, false, false);
    assert(bp_parse(buf, n - 1, &p) == BP_ERR_LENGTH);

    // Code 1: an unknown type, and a P1 type in the first version.
    uint8_t unknown[4] = { 0x77, 0, 0, 0 };
    assert(bp_parse(unknown, 4, &p) == BP_ERR_UNKNOWN_TYPE);
    uint8_t team[4] = { 0x50, 0, 0, 0 };
    assert(bp_parse(team, 4, &p) == BP_ERR_UNKNOWN_TYPE);

    // Code 3: any version other than 0.2 while the major is 0.
    // TURN_SHAPE adds one message and changes no current packet, so the version stays 0.2.
    assert(bp_check_version(0, 2) == BP_OK);
    assert(bp_check_version(0, 3) == BP_ERR_VERSION);
    assert(bp_check_version(0, 1) == BP_ERR_VERSION);
    assert(bp_check_version(1, 2) == BP_ERR_VERSION);

    // Code 7: an unknown codec, a wrong sample rate, or a failure code outside "Failed".
    uint8_t bad_codec[6] = { BP_AGENT_START, 0, 5, 0, 9, 16 };
    assert(bp_parse(bad_codec, 6, &p) == BP_OK);
    bp_agent_start_t s; assert(bp_read_agent_start(&p, &s) == BP_ERR_FIELD);
    uint8_t bad_rate[6] = { BP_AGENT_START, 0, 5, 0, 0, 8 };
    assert(bp_parse(bad_rate, 6, &p) == BP_OK && bp_read_agent_start(&p, &s) == BP_ERR_FIELD);
    uint8_t bad_fail[6] = { BP_AGENT_STATE, 0, 5, 0, BP_AGENT_THINKING, BP_FAIL_GATEWAY };
    bp_agent_state_t st;
    assert(bp_parse(bad_fail, 6, &p) == BP_OK && bp_read_agent_state(&p, &st) == BP_ERR_FIELD);
    uint8_t bad_play[6] = { BP_PLAY_START, 0, 5, 0, 0, 4 };
    bp_play_start_t pl;
    assert(bp_parse(bad_play, 6, &p) == BP_OK && bp_read_play_start(&p, &pl) == BP_ERR_FIELD);
    assert(bp_check_audio_block(9, BP_ADPCM_BLOCK) == BP_ERR_FIELD);

    // Code 2 for an audio block of the wrong size.
    assert(bp_check_audio_block(BP_CODEC_ADPCM, BP_ADPCM_BLOCK - 1) == BP_ERR_LENGTH);
    assert(bp_check_audio_block(BP_CODEC_PCM, BP_PCM_BLOCK) == BP_OK);
}

// Increment 2: LIVE_STATS and RUN_REPORT, doc 07 section 6.
static void live_stats(void) {
    bp_packet_t p;
    const bp_live_stats_t in = { BP_RUN_SKIING, 4, 383, 312, 1850, 24, 151 };
    size_t n = bp_build_live_stats(buf, CAP, &in);
    assert(n == BP_HEADER_SIZE + 12);
    assert(buf[0] == 0x60 && buf[2] == 0 && buf[3] == 0);         // xid 0, seq 0
    assert(buf[6] == 0x7F && buf[7] == 0x01);                       // 383, little-endian
    assert(bp_parse(buf, n, &p) == BP_OK);
    bp_live_stats_t s; assert(bp_read_live_stats(&p, &s) == BP_OK);
    assert(memcmp(&s, &in, sizeof s) == 0);

    // Unknown values pass through unchanged. The screen shows them as "--".
    const bp_live_stats_t unk = { BP_RUN_UNKNOWN, BP_U8_UNKNOWN, BP_U16_UNKNOWN, BP_U16_UNKNOWN,
                                  BP_U16_UNKNOWN, BP_U16_UNKNOWN, BP_U16_UNKNOWN };
    n = bp_build_live_stats(buf, CAP, &unk);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_live_stats(&p, &s) == BP_OK);
    assert(s.speed_dkmh == 0xFFFF && s.run_no == 0xFF && s.turns == 0xFFFF);

    // An unknown run state reads as unknown, with no error.
    buf[4] = 9;
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_live_stats(&p, &s) == BP_OK && s.run_state == BP_RUN_UNKNOWN);
    // A short payload gets code 2. Extra bytes stay allowed.
    assert(bp_parse(buf, n - 1, &p) == BP_ERR_LENGTH);
    assert(bp_parse(buf, n + 3, &p) == BP_OK);
    // The builder refuses a run state that the protocol does not define.
    bp_live_stats_t bad = in; bad.run_state = 4;
    assert(bp_build_live_stats(buf, CAP, &bad) == 0);
}

static void run_report(void) {
    bp_packet_t p;
    bp_run_report_t r;
    const char *problem = "重心偏后";                          // 12 bytes
    const char *tip = "下一趟把双手放在身体前方";                // 36 bytes
    size_t n = bp_build_run_report(buf, CAP, 4, 1850, 24, 82, problem, tip);
    assert(n == BP_HEADER_SIZE + 7 + 12 + 36);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_OK);
    assert(r.run_no == 4 && r.distance_m == 1850 && r.turns == 24 && r.iq == 82);
    assert(r.problem.len == 12 && memcmp(r.problem.ptr, problem, 12) == 0);
    assert(r.tip.len == 36 && memcmp(r.tip.ptr, tip, 36) == 0);

    // Empty texts and an unknown IQ stay valid.
    n = bp_build_run_report(buf, CAP, 1, BP_U16_UNKNOWN, BP_U16_UNKNOWN, BP_U8_UNKNOWN, "", "");
    assert(n == BP_HEADER_SIZE + 7);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_OK);
    assert(r.iq == 0xFF && r.problem.len == 0 && r.tip.len == 0);

    // The longest texts: 12 and 20 Chinese characters. The packet fits at MTU 172.
    char longp[64] = "", longt[96] = "";
    for (int i = 0; i < 12; i++) strcat(longp, "滑");
    for (int i = 0; i < 20; i++) strcat(longt, "雪");
    n = bp_build_run_report(buf, CAP, 2, 100, 10, 100, longp, longt);
    assert(n == BP_HEADER_SIZE + 7 + 36 + 60 && n <= CAP);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_OK);

    // The builder refuses a text above the limit, and an IQ above 100.
    strcat(longt, "雪");
    assert(bp_build_run_report(buf, CAP, 2, 100, 10, 100, longp, longt) == 0);
    assert(bp_build_run_report(buf, CAP, 2, 100, 10, 101, "", "") == 0);

    // The reader gives code 4 for a long text, code 2 for a length past the end,
    // and code 7 for a bad IQ or bad UTF-8.
    uint8_t raw[BP_HEADER_SIZE + 7 + 70] = { BP_RUN_REPORT, 0, 0, 0, 1, 0, 0, 0, 0, 50, 0 };
    memset(raw + 11, 'a', 70);
    assert(bp_parse(raw, sizeof raw, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_ERR_TOO_LARGE);
    raw[10] = 37;                                                   // problem 37 bytes, tip 33
    assert(bp_parse(raw, sizeof raw, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_ERR_TOO_LARGE);
    raw[10] = 71;                                                   // past the end
    assert(bp_parse(raw, sizeof raw, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_ERR_LENGTH);
    raw[10] = 0;
    assert(bp_parse(raw, BP_HEADER_SIZE + 7 + 10, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_OK);
    raw[9] = 101;
    assert(bp_parse(raw, BP_HEADER_SIZE + 7 + 10, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_ERR_FIELD);
    raw[9] = 50;
    raw[11] = 0xE6; raw[12] = 0xBB;                                 // a cut 3-byte character
    assert(bp_parse(raw, BP_HEADER_SIZE + 7 + 2, &p) == BP_OK && bp_read_run_report(&p, &r) == BP_ERR_FIELD);
    assert(bp_parse(raw, BP_HEADER_SIZE + 6, &p) == BP_ERR_LENGTH);
}

static void show_page(void) {
    bp_packet_t p;
    uint8_t page = 0;
    size_t n = bp_build_show_page(buf, CAP, BP_PAGE_REPORT);
    assert(n == BP_HEADER_SIZE + 1 && buf[0] == 0x62 && buf[4] == 3);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_show_page(&p, &page) == BP_OK && page == BP_PAGE_REPORT);
    assert(bp_build_show_page(buf, CAP, BP_PAGE_COACH) == 0);       // Only the Report page.
    uint8_t other[5] = { BP_SHOW_PAGE, 0, 0, 0, BP_PAGE_COACH };
    assert(bp_parse(other, 5, &p) == BP_OK && bp_read_show_page(&p, &page) == BP_ERR_FIELD);
    assert(bp_parse(other, 4, &p) == BP_ERR_LENGTH);
}

static void utf8_rules(void) {
    assert(bp_utf8_valid((const uint8_t *)"", 0));
    assert(bp_utf8_valid((const uint8_t *)"ok 重心", 9));
    const uint8_t overlong[] = { 0xC0, 0xAF };
    const uint8_t surrogate[] = { 0xED, 0xA0, 0x80 };
    const uint8_t lone[] = { 0x80 };
    const uint8_t cut[] = { 0xE9, 0x87 };
    assert(!bp_utf8_valid(overlong, 2) && !bp_utf8_valid(surrogate, 3));
    assert(!bp_utf8_valid(lone, 1) && !bp_utf8_valid(cut, 2));
}

static void trailing_bytes_ignored(void) {
    bp_packet_t p;
    uint8_t ctx[7] = { BP_CONTEXT, 0, 0, 0, BP_MODE_REST, BP_PAGE_COACH, 0xAB };
    assert(bp_parse(ctx, sizeof ctx, &p) == BP_OK);
    bp_context_t c; assert(bp_read_context(&p, &c) == BP_OK && c.mode == 1 && c.page == 1);

    uint8_t unknown_page[6] = { BP_CONTEXT, 0, 0, 0, BP_MODE_REST, 99 };
    assert(bp_parse(unknown_page, 6, &p) == BP_OK && bp_read_context(&p, &c) == BP_OK);
}

static void reader_guards(void) {
    // A packet built by hand, without bp_parse, still cannot overrun.
    uint8_t one = 0;
    bp_packet_t p = { BP_PHONE_STATUS, 0, 0, 0, &one, 1 };
    bp_phone_status_t ps;
    assert(bp_read_phone_status(&p, &ps) == BP_ERR_LENGTH);
    p.type = BP_CONTEXT;
    assert(bp_read_phone_status(&p, &ps) == BP_ERR_STATE);
}

static void xid_rules(void) {
    bp_xid_gen_t d; bp_xid_init_device(&d);
    assert(bp_xid_next(&d) == 0x01);
    for (int i = 0; i < 0x7E - 1; i++) bp_xid_next(&d);
    assert(bp_xid_next(&d) == 0x7F);
    assert(bp_xid_next(&d) == 0x01);                 // wraps inside its own range

    bp_xid_gen_t ph; bp_xid_init_phone(&ph);
    assert(bp_xid_next(&ph) == 0x80);
    for (int i = 0; i < 0xFE - 0x80 - 1; i++) bp_xid_next(&ph);
    assert(bp_xid_next(&ph) == 0xFE);
    assert(bp_xid_next(&ph) == 0x80);

    for (int i = 0; i < 1000; i++) {                 // never 0x00, never 0xFF
        uint8_t a = bp_xid_next(&d), b = bp_xid_next(&ph);
        assert(a >= 0x01 && a <= 0x7F && b >= 0x80 && b <= 0xFE);
    }

    assert(bp_xid_accept(5, 5));
    assert(!bp_xid_accept(5, 4));                    // stale exchange: drop
    assert(!bp_xid_accept(0, 5));                    // after a cancel: drop all
}

static void seq_rules(void) {
    bp_seq_t s; bp_seq_reset(&s);
    assert(bp_seq_accept(&s, 0) == 0);
    assert(bp_seq_accept(&s, 1) == 0);
    assert(bp_seq_accept(&s, 3) == 1);               // one block missing
    bp_seq_reset(&s); s.expect = 255;
    assert(bp_seq_accept(&s, 255) == 0);
    assert(bp_seq_accept(&s, 0) == 0);               // wraps after 255
}

static void type_names(void) {
    assert(strcmp(bp_type_name(BP_AGENT_STATE), "AGENT_STATE") == 0);
    assert(strcmp(bp_type_name(BP_PLAY_END), "PLAY_END") == 0);
    assert(strcmp(bp_type_name(BP_LIVE_STATS), "LIVE_STATS") == 0);
    assert(strcmp(bp_type_name(BP_RUN_REPORT), "RUN_REPORT") == 0);
    assert(strcmp(bp_type_name(0x55), "?") == 0);
}

static void turn_shape(void) {
    uint8_t buf[64];
    bp_packet_t p;
    bp_turn_shape_t r;
    size_t n = bp_build_turn_shape(buf, sizeof buf, 4, BP_TURN_SRC_SCHEMATIC, 255);
    assert(n == 4 + 3 && buf[0] == BP_TURN_SHAPE && buf[2] == 0);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_turn_shape(&p, &r) == BP_OK);
    assert(r.run_no == 4 && r.source == BP_TURN_SRC_SCHEMATIC && r.n_turns == 255);
    assert(bp_build_turn_shape(buf, sizeof buf, 4, 2, 10) == 0);                        // source 2
    buf[5] = 2;
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_turn_shape(&p, &r) == BP_ERR_FIELD);
    const uint8_t short_pkt[] = { BP_TURN_SHAPE, 0, 0, 0, 1, 0 };
    assert(bp_parse(short_pkt, sizeof short_pkt, &p) == BP_ERR_LENGTH);                 // below 3 bytes
    assert(strcmp(bp_type_name(BP_TURN_SHAPE), "TURN_SHAPE") == 0);
}

static void turn_group(void) {
    uint8_t buf[256], pts[2 * BP_GROUP_POINTS_MAX];
    for (int i = 0; i < BP_GROUP_POINTS_MAX; i++) { pts[2 * i] = (uint8_t)(i * 7); pts[2 * i + 1] = (uint8_t)(i * 7 + 1); }
    bp_turn_t turns[BP_GROUP_TURNS];
    for (int i = 0; i < BP_GROUP_TURNS; i++)
        turns[i] = (bp_turn_t){ (uint8_t)(i * 4), (uint8_t)(i * 4 + 4), (uint8_t)(i & 1), 50, 40, 12 };
    bp_packet_t p;
    bp_turn_group_t r;

    // The largest packet: 4 + 48 + 66 payload bytes. It fits the MTU of 172 that voice needs.
    size_t n = bp_build_turn_group(buf, sizeof buf, 4, 31, BP_GROUP_TURNS, turns, BP_GROUP_POINTS_MAX, pts);
    assert(n == 4 + 118 && n + 3 <= BP_MTU_MIN);
    assert(buf[0] == BP_TURN_GROUP && buf[2] == 0);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_turn_group(&p, &r) == BP_OK);
    assert(r.run_no == 4 && r.group == 31 && r.n_turns == 8 && r.n_points == 33);
    assert(r.turns[7].first == 28 && r.turns[7].last == 32 && r.turns[7].side == BP_SIDE_HEEL);
    assert(r.points[2 * 32] == 224 && r.points[2 * 32 + 1] == 225);

    // One turn with unknown metrics.
    turns[0] = (bp_turn_t){ 0, 1, BP_U8_UNKNOWN, BP_U8_UNKNOWN, BP_U8_UNKNOWN, BP_U8_UNKNOWN };
    n = bp_build_turn_group(buf, sizeof buf, 4, 0, 1, turns, 2, pts);
    assert(n == 4 + 4 + 6 + 4);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_turn_group(&p, &r) == BP_OK && r.turns[0].side == BP_U8_UNKNOWN);

    // Errors.
    assert(bp_build_turn_group(buf, sizeof buf, 1, 0, 0, turns, 4, pts) == 0);             // no turn
    assert(bp_build_turn_group(buf, sizeof buf, 1, 0, 9, turns, 33, pts) == 0);            // 9 turns
    assert(bp_build_turn_group(buf, sizeof buf, 1, 0, 1, turns, 34, pts) == 0);            // 34 points
    assert(bp_build_turn_group(buf, sizeof buf, 1, 0, 1, turns, 1, pts) == 0);             // 1 point
    turns[0] = (bp_turn_t){ 3, 3, 0, 1, 1, 1 };
    assert(bp_build_turn_group(buf, sizeof buf, 1, 0, 1, turns, 4, pts) == 0);             // first == last
    turns[0] = (bp_turn_t){ 0, 4, 0, 1, 1, 1 };
    assert(bp_build_turn_group(buf, sizeof buf, 1, 0, 1, turns, 4, pts) == 0);             // last >= points
    turns[0] = (bp_turn_t){ 0, 2, 2, 1, 1, 1 };
    assert(bp_build_turn_group(buf, sizeof buf, 1, 0, 1, turns, 4, pts) == 0);             // side 2
    turns[0] = (bp_turn_t){ 0, 2, 0, 1, 91, 1 };
    assert(bp_build_turn_group(buf, sizeof buf, 1, 0, 1, turns, 4, pts) == 0);             // edge 91

    // The reader checks the same rules on raw bytes.
    turns[0] = (bp_turn_t){ 0, 2, 0, 1, 1, 1 };
    n = bp_build_turn_group(buf, sizeof buf, 1, 0, 1, turns, 4, pts);
    buf[4 + 4 + 4] = 91;                                                                     // edge 91
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_turn_group(&p, &r) == BP_ERR_FIELD);
    buf[4 + 4 + 4] = 1;
    assert(bp_parse(buf, n - 1, &p) == BP_OK && bp_read_turn_group(&p, &r) == BP_ERR_LENGTH); // half a point
    buf[4 + 3] = 34;                                                                         // 34 points
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_turn_group(&p, &r) == BP_ERR_TOO_LARGE);
    assert(strcmp(bp_type_name(BP_TURN_GROUP), "TURN_GROUP") == 0);
}

static void turn_request(void) {
    uint8_t buf[16];
    bp_packet_t p;
    bp_turn_request_t r;
    size_t n = bp_build_turn_request(buf, sizeof buf, 4, 2, 3);
    assert(n == 4 + 3 && buf[0] == BP_TURN_REQUEST && buf[4] == 4 && buf[5] == 2 && buf[6] == 3);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_turn_request(&p, &r) == BP_OK);
    assert(r.run_no == 4 && r.first_group == 2 && r.count == 3);
    assert(bp_build_turn_request(buf, sizeof buf, 4, 0, 0) == 0 && bp_build_turn_request(buf, sizeof buf, 4, 0, 4) == 0);
    n = bp_build_turn_request(buf, sizeof buf, 4, 0, 1);
    buf[6] = 4;
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_turn_request(&p, &r) == BP_ERR_FIELD);
    assert(strcmp(bp_type_name(BP_TURN_REQUEST), "TURN_REQUEST") == 0);
}

static void popup(void) {
    uint8_t buf[200];
    bp_packet_t p;
    bp_popup_t r;
    uint8_t reason = 9;
    size_t n = bp_build_popup(buf, sizeof buf, 7, BP_POPUP_FALL, true, "检测到摔倒", "需要帮助吗");
    assert(n == 4 + 3 + 15 + 15 && buf[0] == BP_POPUP && buf[2] == 7);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_popup(&p, &r) == BP_OK);
    assert(r.kind == BP_POPUP_FALL && r.audio && r.title.len == 15 && r.body.len == 15);
    assert(memcmp(r.title.ptr, "检测到摔倒", 15) == 0 && memcmp(r.body.ptr, "需要帮助吗", 15) == 0);
    n = bp_build_popup(buf, sizeof buf, 8, BP_POPUP_BADGE, false, "", NULL);
    assert(n == 7 && bp_parse(buf, n, &p) == BP_OK && bp_read_popup(&p, &r) == BP_OK);
    assert(r.kind == BP_POPUP_BADGE && !r.audio && r.title.len == 0 && r.body.len == 0);
    assert(bp_build_popup(buf, sizeof buf, 1, 0, false, "a", "b") == 0);
    assert(bp_build_popup(buf, sizeof buf, 1, 5, false, "a", "b") == 0);
    char long_title[BP_POPUP_TITLE_MAX + 2];
    memset(long_title, 'a', sizeof long_title - 1);
    long_title[sizeof long_title - 1] = 0;
    assert(bp_build_popup(buf, sizeof buf, 1, 1, false, long_title, "") == 0);
    n = bp_build_popup(buf, sizeof buf, 1, BP_POPUP_VOICE, false, "ab", "cd");
    buf[5] = 2;
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_popup(&p, &r) == BP_ERR_FIELD);
    buf[5] = 0; buf[6] = 9;
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_popup(&p, &r) == BP_ERR_LENGTH);
    buf[6] = 2; buf[7] = 0xFF;
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_popup(&p, &r) == BP_ERR_FIELD);
    n = bp_build_popup_end(buf, sizeof buf, 7, BP_POPUP_END_REPLACED);
    assert(n == 5 && buf[0] == BP_POPUP_END && buf[2] == 7);
    assert(bp_parse(buf, n, &p) == BP_OK && bp_read_popup_end(&p, &reason) == BP_OK && reason == BP_POPUP_END_REPLACED);
    assert(bp_build_popup_end(buf, sizeof buf, 7, 3) == 0);
    buf[4] = 3;
    assert(bp_read_popup_end(&p, &reason) == BP_ERR_FIELD);
    assert(strcmp(bp_type_name(BP_POPUP), "POPUP") == 0 && strcmp(bp_type_name(BP_POPUP_END), "POPUP_END") == 0);
}

int main(void) {
    popup();
    type_names();
    turn_shape();
    turn_group();
    turn_request();
    roundtrip_link();
    roundtrip_agent_and_audio();
    error_codes();
    live_stats();
    run_report();
    utf8_rules();
    show_page();
    trailing_bytes_ignored();
    reader_guards();
    xid_rules();
    seq_rules();
    printf("ble protocol tests: PASS\n");
    return 0;
}
