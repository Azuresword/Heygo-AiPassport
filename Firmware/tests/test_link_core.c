// tests/test_link_core.c
// Host test for the protocol session in main/link/link_core.c.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link_core.h"

static int s_fail;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); s_fail++; } } while (0)

// ---------------------------------------------------------------- fake I/O
typedef struct { link_lane_t lane; uint8_t buf[LINK_PACKET_MAX]; size_t len; } sent_t;
static sent_t s_sent[64];
static int s_nsent, s_drops, s_play_start, s_play_end, s_down, s_missing;
static link_voice_t s_voice = LINK_VOICE_NO_LINK;
static int s_voice_calls;
static uint8_t s_state, s_failure;
static char s_text[LINK_TEXT_MAX + 1];
static int s_states;

static void io_send(void *c, link_lane_t lane, const uint8_t *pkt, size_t len) {
    (void)c;
    if (s_nsent < 64) { s_sent[s_nsent].lane = lane; memcpy(s_sent[s_nsent].buf, pkt, len); s_sent[s_nsent].len = len; }
    s_nsent++;
}
static void io_drop(void *c) { (void)c; s_drops++; }
static void io_voice(void *c, link_voice_t v) { (void)c; s_voice = v; s_voice_calls++; }
static void io_state(void *c, uint8_t st, uint8_t f, const char *t) {
    (void)c; s_state = st; s_failure = f; snprintf(s_text, sizeof s_text, "%s", t); s_states++;
}
static void io_play_start(void *c) { (void)c; s_play_start++; }
static void io_down(void *c, const uint8_t *b, size_t n, uint8_t miss) { (void)c; (void)b; (void)n; s_down++; s_missing += miss; }
static void io_play_end(void *c) { (void)c; s_play_end++; }
static int s_drops_rx;
static uint8_t s_drop_type, s_drop_xid, s_drop_cur;
static void io_dropped(void *c, uint8_t t, uint8_t x, uint8_t cur) { (void)c; s_drops_rx++; s_drop_type = t; s_drop_xid = x; s_drop_cur = cur; }
static int s_lives, s_reports;
static bp_live_stats_t s_live;
static link_report_t s_report;
static void io_live(void *c, const bp_live_stats_t *s) { (void)c; s_lives++; s_live = *s; }
static void io_report(void *c, const link_report_t *r) { (void)c; s_reports++; s_report = *r; }
static int s_pages;
static uint8_t s_page;
static void io_page(void *c, uint8_t page) { (void)c; s_pages++; s_page = page; }
static int s_turn_calls;
static void io_turns(void *c, const link_turns_t *t) { (void)c; (void)t; s_turn_calls++; }

static const link_io_t IO = { NULL, io_send, io_drop, io_voice, io_state, io_play_start, io_down, io_play_end, io_dropped,
                              io_live, io_report, io_page, io_turns };

static void clear(void) { s_nsent = 0; }
static const sent_t *last(void) { return s_nsent ? &s_sent[s_nsent - 1] : NULL; }
static int count_type(uint8_t type) { int n = 0; for (int i = 0; i < s_nsent && i < 64; i++) n += s_sent[i].buf[0] == type; return n; }
static const sent_t *find_type(uint8_t type) {
    for (int i = 0; i < s_nsent && i < 64; i++) if (s_sent[i].buf[0] == type) return &s_sent[i];
    return NULL;
}

static void rx(link_core_t *l, const uint8_t *p, size_t n, uint32_t now) { link_on_rx(l, p, n, now); }

static void ack(link_core_t *l, uint8_t major, uint8_t minor, uint32_t now) {
    uint8_t p[32];
    size_t n = bp_build_hello_ack(p, sizeof p, major, minor, 1760000000u, "app 1.0");
    rx(l, p, n, now);
}

// Bring a fresh core to voice OK at time t. Returns t.
static uint32_t ready(link_core_t *l, uint32_t t) {
    link_init(l, &IO, "fw 0.2");
    link_on_connect(l, t);
    link_on_mtu(l, 247, t);
    link_on_subscribe(l, true, t);
    ack(l, 0, 2, t + 10);
    return t + 10;
}

// ---------------------------------------------------------------- tests
static void test_handshake(void) {
    link_core_t l;
    link_init(&l, &IO, "fw 0.2");
    CHECK(link_voice(&l) == LINK_VOICE_NO_LINK, "starts without a link");
    clear();
    link_on_connect(&l, 1000);
    CHECK(s_nsent == 0, "no HELLO before the subscription");
    link_on_subscribe(&l, true, 1000);
    const sent_t *h = last();
    CHECK(h && h->buf[0] == BP_HELLO && h->lane == LINK_LANE_CONTROL, "HELLO after the subscription");
    CHECK(h && h->buf[4] == 0 && h->buf[5] == 2 && h->buf[6] == BP_CODEC_MASK_ADPCM, "HELLO carries 0.2 and ADPCM only");
    CHECK(h && h->buf[7] == 23 && h->buf[8] == 0, "HELLO carries the current MTU");
    CHECK(s_voice == LINK_VOICE_WAIT_HELLO, "waits for HELLO_ACK");
    link_tick(&l, 2999);
    CHECK(s_voice == LINK_VOICE_WAIT_HELLO, "still waits at 1999 ms");
    link_tick(&l, 3000);
    CHECK(s_voice == LINK_VOICE_NO_ACK, "no HELLO_ACK after 2 s");

    clear();
    ack(&l, 0, 2, 3100);
    CHECK(s_voice == LINK_VOICE_MTU, "a late HELLO_ACK works, but MTU 23 blocks voice");
    CHECK(find_type(BP_CONTEXT) && find_type(BP_DEVICE_STATUS), "CONTEXT and DEVICE_STATUS after HELLO_ACK");
    link_on_mtu(&l, 171, 3100);
    CHECK(s_voice == LINK_VOICE_MTU, "MTU 171 blocks voice");
    link_on_mtu(&l, 172, 3100);
    CHECK(s_voice == LINK_VOICE_OK, "MTU 172 allows voice");
}

static void test_version(void) {
    link_core_t l;
    link_init(&l, &IO, "fw");
    link_on_connect(&l, 0);
    link_on_mtu(&l, 247, 0);
    link_on_subscribe(&l, true, 0);
    clear();
    ack(&l, 0, 1, 50);
    const sent_t *e = last();
    CHECK(e && e->buf[0] == BP_ERROR && e->buf[4] == BP_ERR_VERSION && e->buf[5] == BP_HELLO_ACK, "ERROR 3 for version 0.1");
    CHECK(s_voice == LINK_VOICE_VERSION, "a version mismatch blocks voice");
    CHECK(link_agent_start(&l) == 0, "no exchange without voice");
    ack(&l, 0, 2, 60);
    CHECK(s_voice == LINK_VOICE_OK, "a correct HELLO_ACK recovers");
}

static void test_exchange(void) {
    link_core_t l;
    uint32_t t = ready(&l, 0);
    CHECK(s_voice == LINK_VOICE_OK, "ready");
    clear();
    uint8_t xid = link_agent_start(&l);
    CHECK(xid == 0x01, "the first device xid is 0x01");
    CHECK(last()->buf[0] == BP_AGENT_START && last()->buf[2] == 0x01 && last()->buf[4] == BP_CODEC_ADPCM &&
          last()->buf[5] == 16 && last()->lane == LINK_LANE_CONTROL, "AGENT_START in the control lane");
    uint8_t block[BP_ADPCM_BLOCK] = { 0 };
    link_audio_up(&l, block);
    link_audio_up(&l, block);
    CHECK(s_nsent == 3 && s_sent[1].buf[0] == BP_AUDIO_UP && s_sent[1].buf[3] == 0 && s_sent[2].buf[3] == 1,
          "AUDIO_UP seq 0 and 1");
    CHECK(s_sent[2].len == 168 && s_sent[2].lane == LINK_LANE_AUDIO, "168-byte audio packets in the audio lane");
    link_agent_end(&l, BP_END_LIMIT);
    CHECK(last()->buf[0] == BP_AGENT_END && last()->buf[4] == BP_END_LIMIT && last()->lane == LINK_LANE_AUDIO,
          "AGENT_END follows the audio in the audio lane");

    uint8_t p[LINK_PACKET_MAX];
    size_t n = bp_build_agent_state(p, sizeof p, 0x02, BP_AGENT_RECOGNIZING, 0, "stale");
    s_states = 0;
    rx(&l, p, n, t + 100);
    CHECK(s_states == 0, "AGENT_STATE with another xid is dropped");
    CHECK(s_drops_rx == 1 && s_drop_type == BP_AGENT_STATE && s_drop_xid == 0x02 && s_drop_cur == xid,
          "the drop is reported with both xids");
    n = bp_build_agent_state(p, sizeof p, xid, BP_AGENT_RECOGNIZING, 0, "上一趟怎么样");
    rx(&l, p, n, t + 110);
    CHECK(s_states == 1 && s_state == BP_AGENT_RECOGNIZING && strcmp(s_text, "上一趟怎么样") == 0, "AGENT_STATE text arrives");

    clear();
    n = bp_build_play_start(p, sizeof p, xid, BP_CHANNEL_AGENT, BP_CODEC_PCM, NULL);
    rx(&l, p, n, t + 120);
    CHECK(s_play_start == 0 && last() && last()->buf[0] == BP_ERROR && last()->buf[4] == BP_ERR_FIELD,
          "PCM was not offered: ERROR 7");
    n = bp_build_play_start(p, sizeof p, xid, BP_CHANNEL_AGENT, BP_CODEC_ADPCM, NULL);
    rx(&l, p, n, t + 130);
    CHECK(s_play_start == 1, "PLAY_START opens the answer");

    uint8_t blk[BP_ADPCM_BLOCK] = { 0 };
    n = bp_build_audio(p, sizeof p, BP_AUDIO_DOWN, xid, 0, blk, 100);
    clear();
    rx(&l, p, n, t + 140);
    CHECK(s_down == 0 && last() && last()->buf[4] == BP_ERR_LENGTH, "a short block gets ERROR 2");
    n = bp_build_audio(p, sizeof p, BP_AUDIO_DOWN, xid, 0, blk, BP_ADPCM_BLOCK);
    rx(&l, p, n, t + 150);
    n = bp_build_audio(p, sizeof p, BP_AUDIO_DOWN, xid, 2, blk, BP_ADPCM_BLOCK);
    rx(&l, p, n, t + 160);
    CHECK(s_down == 2 && s_missing == 1, "seq 0 then 2 reports one missing block");
    n = bp_build_play_end(p, sizeof p, xid);
    rx(&l, p, n, t + 170);
    CHECK(s_play_end == 1, "PLAY_END closes the answer");

    link_agent_done(&l);
    n = bp_build_agent_state(p, sizeof p, xid, BP_AGENT_DONE, 0, NULL);
    s_states = 0;
    rx(&l, p, n, t + 180);
    CHECK(s_states == 0, "after the answer, packets of the old xid are dropped");
    CHECK(link_agent_start(&l) == 0x02, "the next exchange uses 0x02");
}

static void test_cancel(void) {
    link_core_t l;
    uint32_t t = ready(&l, 0);
    uint8_t xid = link_agent_start(&l);
    s_drops = 0;
    clear();
    link_agent_cancel(&l);
    CHECK(s_drops == 1, "cancel drops the queued uplink audio");
    CHECK(last() && last()->buf[0] == BP_AGENT_CANCEL && last()->buf[2] == xid && last()->lane == LINK_LANE_CONTROL,
          "AGENT_CANCEL in the control lane");
    uint8_t p[LINK_PACKET_MAX];
    size_t n = bp_build_play_start(p, sizeof p, xid, BP_CHANNEL_AGENT, BP_CODEC_ADPCM, NULL);
    int before = s_play_start;
    rx(&l, p, n, t + 10);
    CHECK(s_play_start == before, "a late answer of a canceled exchange never starts");
    CHECK(s_drop_type == BP_PLAY_START && s_drop_cur == 0, "the late PLAY_START is reported as a drop");
    uint8_t block[BP_ADPCM_BLOCK] = { 0 };
    clear();
    link_audio_up(&l, block);
    CHECK(s_nsent == 0, "no AUDIO_UP after cancel");
}

static void test_silence(void) {
    link_core_t l;
    uint32_t t = ready(&l, 100000);
    CHECK(s_voice == LINK_VOICE_OK, "voice is OK right after the handshake, long after boot");
    link_agent_start(&l);
    clear();
    link_tick(&l, t + LINK_PHONE_SILENT_MS - 1);
    CHECK(s_voice == LINK_VOICE_OK, "OK before 15 s of silence");
    link_tick(&l, t + LINK_PHONE_SILENT_MS);
    CHECK(s_voice == LINK_VOICE_SILENT, "15 s of silence blocks voice");
    CHECK(find_type(BP_AGENT_CANCEL) != NULL, "silence closes the open exchange");
    uint8_t p[16];
    size_t n = bp_build_phone_status(p, sizeof p, 1760000100u, true, true, false);
    rx(&l, p, n, t + LINK_PHONE_SILENT_MS + 5);
    CHECK(s_voice == LINK_VOICE_OK, "PHONE_STATUS recovers voice");
    CHECK(l.utc == 1760000100u, "PHONE_STATUS updates the UTC time");
}

static void test_errors(void) {
    link_core_t l;
    uint32_t t = ready(&l, 0);
    clear();
    const uint8_t unknown[] = { 0x55, 0, 0x81, 0 };
    rx(&l, unknown, sizeof unknown, t);
    CHECK(last() && last()->buf[0] == BP_ERROR && last()->buf[4] == BP_ERR_UNKNOWN_TYPE &&
          last()->buf[5] == 0x55 && last()->buf[6] == 0x81, "unknown type: ERROR 1 with the type and the xid");
    const uint8_t shortp[] = { BP_HELLO_ACK, 0 };
    rx(&l, shortp, sizeof shortp, t);
    CHECK(last()->buf[4] == BP_ERR_LENGTH, "a packet shorter than the header: ERROR 2");
    uint8_t p[16];
    size_t n = bp_build_agent_start(p, sizeof p, 0x80, BP_CODEC_ADPCM);
    rx(&l, p, n, t);
    CHECK(last()->buf[4] == BP_ERR_STATE && last()->buf[5] == BP_AGENT_START, "a device type from the phone: ERROR 6");
    n = bp_build_play_start(p, sizeof p, 0x81, BP_CHANNEL_TEAM, BP_CODEC_ADPCM, "Li");
    int before = s_play_start;
    rx(&l, p, n, t);
    CHECK(s_play_start == before, "Team speech is P1 and is ignored");
}

static void test_reconnect_and_status(void) {
    link_core_t l;
    uint32_t t = ready(&l, 0);
    link_agent_start(&l);
    link_agent_start(&l);
    link_on_disconnect(&l, t + 1);
    CHECK(s_voice == LINK_VOICE_NO_LINK, "disconnect blocks voice");
    link_on_connect(&l, t + 2);
    link_on_mtu(&l, 247, t + 2);
    link_on_subscribe(&l, true, t + 2);
    ack(&l, 0, 2, t + 3);
    CHECK(link_agent_start(&l) == 0x01, "a new connection resets the xid to 0x01");

    clear();
    link_set_device(&l, 80, BP_MODE_REST, BP_PAGE_COACH, 100, t + 10);
    CHECK(count_type(BP_CONTEXT) == 1 && count_type(BP_DEVICE_STATUS) == 1, "a page change sends CONTEXT and DEVICE_STATUS");
    clear();
    link_set_device(&l, 80, BP_MODE_REST, BP_PAGE_COACH, 90, t + 20);
    CHECK(s_nsent == 0, "a heap change alone waits for the periodic status");
    link_set_device(&l, 79, BP_MODE_REST, BP_PAGE_COACH, 90, t + 30);
    CHECK(count_type(BP_DEVICE_STATUS) == 1 && count_type(BP_CONTEXT) == 0, "a battery change sends DEVICE_STATUS only");
    clear();
    link_tick(&l, t + 30 + LINK_DEVICE_STATUS_MS - 1);
    CHECK(count_type(BP_DEVICE_STATUS) == 0, "no periodic status before 30 s");
    // Keep the phone alive so that the silence rule does not fire.
    uint8_t p[16];
    size_t n = bp_build_phone_status(p, sizeof p, 0, true, true, false);
    rx(&l, p, n, t + 30 + LINK_DEVICE_STATUS_MS - 1);
    link_tick(&l, t + 30 + LINK_DEVICE_STATUS_MS);
    CHECK(count_type(BP_DEVICE_STATUS) == 1, "DEVICE_STATUS every 30 s");
    const sent_t *st = find_type(BP_DEVICE_STATUS);
    CHECK(st && st->buf[4] == 79 && st->buf[5] == BP_MODE_REST && st->buf[6] == BP_PAGE_COACH && st->buf[7] == 90,
          "DEVICE_STATUS carries battery, mode, page, and heap");
}

// Increment 2: LIVE_STATS and RUN_REPORT, doc 07 sections 6 and 8.
static void test_live_data(void) {
    link_core_t l;
    uint8_t p[LINK_PACKET_MAX];
    const bp_live_stats_t in = { BP_RUN_SKIING, 3, 383, 312, 1850, 24, 151 };
    size_t n = bp_build_live_stats(p, sizeof p, &in);
    bool stale = false;
    s_lives = 0;

    // Before HELLO_ACK the device ignores the data.
    link_init(&l, &IO, "fw");
    link_on_connect(&l, 0);
    link_on_subscribe(&l, true, 0);
    rx(&l, p, n, 5);
    CHECK(s_lives == 0 && link_live(&l, 5, &stale) == NULL && stale, "no data before HELLO_ACK");

    // A wrong version also blocks the data.
    ack(&l, 0, 1, 10);
    rx(&l, p, n, 20);
    CHECK(s_lives == 0, "no data with a version mismatch");

    // MTU 23 blocks voice, but not the data: LIVE_STATS needs 31 only.
    ack(&l, 0, 2, 30);
    CHECK(s_voice == LINK_VOICE_MTU, "MTU 23 blocks voice");
    clear();
    rx(&l, p, n, 1000);
    CHECK(s_lives == 1 && s_live.distance_m == 1850 && s_live.turns == 24, "LIVE_STATS reaches the app");
    CHECK(s_nsent == 0, "the device sends no reply to LIVE_STATS");
    const bp_live_stats_t *got = link_live(&l, 1000 + LINK_LIVE_STALE_MS - 1, &stale);
    CHECK(got && got->time_s == 151 && !stale, "fresh before 5 s");
    link_live(&l, 1000 + LINK_LIVE_STALE_MS, &stale);
    CHECK(stale, "stale after 5 s with no packet");

    // A new packet replaces the old one completely.
    const bp_live_stats_t next = { BP_RUN_ENDED, 3, BP_U16_UNKNOWN, 320, 1900, 25, 160 };
    n = bp_build_live_stats(p, sizeof p, &next);
    rx(&l, p, n, 7000);
    got = link_live(&l, 7000, &stale);
    CHECK(got && got->run_state == BP_RUN_ENDED && got->speed_dkmh == 0xFFFF && !stale, "the new packet replaces the old");

    // A short payload gets ERROR 2 and keeps the old packet.
    clear();
    rx(&l, p, n - 1, 7100);
    const sent_t *e = last();
    CHECK(e && e->buf[0] == BP_ERROR && e->buf[4] == BP_ERR_LENGTH && e->buf[5] == BP_LIVE_STATS, "ERROR 2 for a short LIVE_STATS");
    got = link_live(&l, 7100, &stale);
    CHECK(got && got->distance_m == 1900, "a short packet keeps the old data");

    // RUN_REPORT: copied texts, closed by a zero byte.
    s_reports = 0;
    uint32_t age = 0;
    CHECK(link_report(&l, 7200, &age) == NULL, "no report before the first one");
    n = bp_build_run_report(p, sizeof p, 3, 1900, 25, 82, "重心偏后", "下一趟把双手放在身体前方");
    rx(&l, p, n, 8000);
    const link_report_t *r = link_report(&l, 8000 + LINK_DATA_AGE_MS, &age);
    CHECK(s_reports == 1 && r && r->run_no == 3 && r->iq == 82 && r->turns == 25, "RUN_REPORT reaches the app");
    CHECK(r && strcmp(r->problem, "重心偏后") == 0 && strcmp(r->tip, "下一趟把双手放在身体前方") == 0, "the texts arrive whole");
    CHECK(age == LINK_DATA_AGE_MS, "the report age uses the local receive time");

    // A bad report gets an ERROR and keeps the old report.
    clear();
    p[BP_HEADER_SIZE + 5] = 201;
    rx(&l, p, n, 8100);
    e = last();
    CHECK(e && e->buf[4] == BP_ERR_FIELD && e->buf[5] == BP_RUN_REPORT, "ERROR 7 for an IQ above 200");
    r = link_report(&l, 8100, &age);
    CHECK(s_reports == 1 && r && r->iq == 82, "a bad report keeps the old report");

    // SHOW_PAGE: the Report page reaches the app. Another page gets ERROR 7.
    s_pages = 0;
    n = bp_build_show_page(p, sizeof p, BP_PAGE_REPORT);
    rx(&l, p, n, 8200);
    CHECK(s_pages == 1 && s_page == BP_PAGE_REPORT, "SHOW_PAGE reaches the app");
    clear();
    p[BP_HEADER_SIZE] = BP_PAGE_COACH;
    rx(&l, p, n, 8300);
    e = last();
    CHECK(s_pages == 1 && e && e->buf[4] == BP_ERR_FIELD && e->buf[5] == BP_SHOW_PAGE, "ERROR 7 for another page");

    // A disconnect keeps both, so that the screen can still show them.
    link_on_disconnect(&l, 9000);
    CHECK(link_live(&l, 9000, &stale) != NULL && link_report(&l, 9000, &age) != NULL, "a disconnect keeps the data");
}

// A TURN_GROUP of n_turns turns with 4 points each turn.
static size_t group_pkt(uint8_t *p, size_t cap, uint8_t run, uint8_t group, uint8_t n_turns, uint8_t edge0) {
    bp_turn_t turns[BP_GROUP_TURNS];
    uint8_t pts[2 * BP_GROUP_POINTS_MAX];
    for (int i = 0; i < n_turns; i++) turns[i] = (bp_turn_t){ (uint8_t)(4 * i), (uint8_t)(4 * i + 4), (uint8_t)(i & 1), 50, 40, 12 };
    turns[0].edge_deg = edge0;
    const uint8_t n = (uint8_t)(4 * n_turns + 1);
    for (int i = 0; i < n; i++) { pts[2 * i] = (uint8_t)(i * 7); pts[2 * i + 1] = group; }
    return bp_build_turn_group(p, cap, run, group, n_turns, turns, n, pts);
}

// The last TURN_REQUEST that the device sent, as first and count. Returns false when none.
static bool last_request(uint8_t *first, uint8_t *count) {
    const sent_t *s = last();
    if (!s || s->buf[0] != BP_TURN_REQUEST) return false;
    *first = s->buf[5];
    *count = s->buf[6];
    return true;
}

static void test_turn_shape(void) {
    link_core_t l;
    uint8_t p[256], q[256], first = 0, count = 0;
    size_t n = bp_build_turn_shape(p, sizeof p, 4, BP_TURN_SRC_MEASURED, 44);   // 6 groups

    // Before HELLO_ACK the data is not allowed.
    link_init(&l, &IO, "fw");
    link_on_connect(&l, 0);
    link_on_mtu(&l, 247, 0);
    link_on_subscribe(&l, true, 0);
    s_turn_calls = 0;
    rx(&l, p, n, 5);
    CHECK(s_turn_calls == 0 && link_turns(&l) == NULL, "no turn shape before HELLO_ACK");

    uint32_t t = ready(&l, 0);
    size_t m = group_pkt(q, sizeof q, 4, 0, 8, 40);
    clear();
    rx(&l, q, m, t + 1);
    CHECK(last() && last()->buf[4] == BP_ERR_STATE && last()->buf[5] == BP_TURN_GROUP, "a group before the shape gets ERROR 6");

    // The shape asks for the first three groups at once: 24 turns.
    clear();
    rx(&l, p, n, t + 1);
    const link_turns_t *got = link_turns(&l);
    CHECK(s_turn_calls == 1 && got && got->n_turns == 44 && got->n_groups == 6, "the turn shape arrives");
    CHECK(last_request(&first, &count) && first == 0 && count == 3, "the shape asks for groups 0 to 2");

    for (uint8_t g = 0; g < 3; g++) {
        m = group_pkt(q, sizeof q, 4, g, 8, 40);
        rx(&l, q, m, t + 2);
    }
    CHECK(s_turn_calls == 4 && link_turns_group(got, 0) && link_turns_group(got, 2), "three groups are cached");
    const link_turn_group_t *g0 = link_turns_group(got, 0);
    CHECK(g0 && g0->n_turns == 8 && g0->n_points == 33 && g0->points[32][0] == 224 && g0->turns[7].side == BP_SIDE_HEEL,
          "the group is copied");

    // The screen moves to group 2: keep 1 to 3, drop 0, and ask for 3.
    clear();
    link_turns_focus(&l, 2, t + 3);
    CHECK(!link_turns_group(got, 0) && link_turns_group(got, 1) && link_turns_group(got, 2), "group 0 is dropped");
    CHECK(last_request(&first, &count) && first == 3 && count == 1, "the device asks for group 3 only");

    // No answer: one more request after 1 s, and no third one.
    clear();
    link_tick(&l, t + 3 + LINK_TURN_RETRY_MS - 1);
    CHECK(!last(), "no retry before 1 s");
    link_tick(&l, t + 3 + LINK_TURN_RETRY_MS);
    CHECK(last_request(&first, &count) && first == 3 && count == 1, "one retry after 1 s");
    clear();
    link_tick(&l, t + 3 + 3 * LINK_TURN_RETRY_MS);
    CHECK(!last(), "no second retry");

    // A late answer for a group outside the window is dropped without an error.
    clear();
    m = group_pkt(q, sizeof q, 4, 0, 8, 40);
    rx(&l, q, m, t + 4000);
    CHECK(!last() && !link_turns_group(got, 0), "a group outside the window is dropped");

    // The last group holds the rest of the turns: 44 - 40 = 4.
    link_turns_focus(&l, 5, t + 4001);
    CHECK(last_request(&first, &count) && first == 3 && count == 3, "the last window is groups 3 to 5");
    clear();
    m = group_pkt(q, sizeof q, 4, 5, 8, 40);
    rx(&l, q, m, t + 4002);
    CHECK(last() && last()->buf[4] == BP_ERR_FIELD, "a last group with 8 turns gets ERROR 7");
    m = group_pkt(q, sizeof q, 4, 5, 4, 40);
    rx(&l, q, m, t + 4002);
    CHECK(link_turns_group(got, 5) && link_turns_group(got, 5)->n_turns == 4, "the last group has 4 turns");
    clear();
    m = group_pkt(q, sizeof q, 4, 6, 4, 40);
    rx(&l, q, m, t + 4003);
    CHECK(last() && last()->buf[4] == BP_ERR_FIELD, "a group past the run gets ERROR 7");
    m = group_pkt(q, sizeof q, 5, 4, 8, 40);
    rx(&l, q, m, t + 4003);
    CHECK(last() && last()->buf[4] == BP_ERR_STATE, "a group of another run gets ERROR 6");
    m = group_pkt(q, sizeof q, 4, 4, 8, 40);
    q[4 + 4 + 4] = 99;   // edge 99 in the first turn
    rx(&l, q, m, t + 4003);
    CHECK(last() && last()->buf[4] == BP_ERR_FIELD && !link_turns_group(got, 4), "a bad edge angle gets ERROR 7");

    // A short run fits in one group.
    clear();
    n = bp_build_turn_shape(p, sizeof p, 6, BP_TURN_SRC_SCHEMATIC, 5);
    rx(&l, p, n, t + 5000);
    CHECK(got->n_groups == 1 && !link_turns_group(got, 5), "a new shape clears the groups");
    CHECK(last_request(&first, &count) && first == 0 && count == 1, "a short run asks for one group");
    link_on_disconnect(&l, t + 5001);
    CHECK(link_turns(&l) != NULL, "a disconnect keeps the shape for the screen");
}

// The App team request of 2026-10-17: the Turns page shows the run of the last RUN_REPORT.
static size_t report_pkt(uint8_t *p, size_t cap, uint8_t run) {
    return bp_build_run_report(p, cap, run, 900, 20, 127, "", "");
}

static void test_turn_report_sync(void) {
    link_core_t l;
    uint8_t p[256], q[256], first = 0, count = 0;
    link_init(&l, &IO, "fw");
    uint32_t t = ready(&l, 0);
    const link_turns_t *got = NULL;

    // A report of a new run clears the cache of run 1, waits, and asks for nothing.
    size_t n = bp_build_turn_shape(p, sizeof p, 1, BP_TURN_SRC_MEASURED, 20);
    rx(&l, p, n, t + 1);
    size_t m = group_pkt(q, sizeof q, 1, 0, 8, 40);
    rx(&l, q, m, t + 2);
    clear();
    s_turn_calls = 0;
    n = report_pkt(p, sizeof p, 2);
    rx(&l, p, n, t + 3);
    got = link_turns(&l);
    CHECK(got && got->run_no == 2 && got->n_turns == 0 && got->n_groups == 0 && got->waiting,
          "F1: a report of run 2 gives a waiting cache of run 2 with 0 turns");
    CHECK(!link_turns_group(got, 0) && s_turn_calls == 1, "F1: the groups of run 1 are gone, and the screen hears of it");
    CHECK(!last_request(&first, &count), "F1: no TURN_REQUEST for a waiting cache");

    // The same report again keeps the cache.
    rx(&l, p, n, t + 4);
    CHECK(got->run_no == 2 && got->waiting && s_turn_calls == 1, "F1: a second report of run 2 keeps the cache");

    // A group of the old run gets ERROR 6, and the cache stays.
    clear();
    m = group_pkt(q, sizeof q, 1, 0, 8, 40);
    rx(&l, q, m, t + 5);
    CHECK(last() && last()->buf[4] == BP_ERR_STATE && last()->buf[5] == BP_TURN_GROUP, "F1: a group of run 1 gets ERROR 6");
    CHECK(got->run_no == 2 && got->waiting && got->n_turns == 0, "F1: the cache does not change");

    // The shape of run 2 ends the wait and asks for groups 0 to 2.
    clear();
    n = bp_build_turn_shape(p, sizeof p, 2, BP_TURN_SRC_MEASURED, 20);
    rx(&l, p, n, t + 6);
    CHECK(!got->waiting && got->n_turns == 20 && got->n_groups == 3, "F1: the shape of run 2 ends the wait");
    CHECK(last_request(&first, &count) && first == 0 && count == 3, "F1: then the device asks for groups 0 to 2");

    // A shape of 0 turns: no wait, nothing to ask for.
    n = report_pkt(p, sizeof p, 3);
    rx(&l, p, n, t + 7);
    clear();
    n = bp_build_turn_shape(p, sizeof p, 3, BP_TURN_SRC_MEASURED, 0);
    rx(&l, p, n, t + 8);
    CHECK(got->run_no == 3 && !got->waiting && got->n_turns == 0 && !last(), "F2: a shape of 0 turns ends the wait");
    // A later shape with the real count replaces it, as the App sends it.
    n = bp_build_turn_shape(p, sizeof p, 3, BP_TURN_SRC_MEASURED, 9);
    rx(&l, p, n, t + 9);
    CHECK(got->n_turns == 9 && got->n_groups == 2, "a later shape of the same run gives the real count");

    // No shape at all: after LINK_TURN_WAIT_MS the run counts as a run with no turn data.
    n = report_pkt(p, sizeof p, 4);
    rx(&l, p, n, t + 10);
    s_turn_calls = 0;
    link_tick(&l, t + 10 + LINK_TURN_WAIT_MS - 1);
    CHECK(got->waiting && s_turn_calls == 0, "the wait goes on until the time limit");
    link_tick(&l, t + 10 + LINK_TURN_WAIT_MS);
    CHECK(!got->waiting && got->run_no == 4 && got->n_turns == 0 && s_turn_calls == 1, "the wait ends at the time limit");
    n = bp_build_turn_shape(p, sizeof p, 4, BP_TURN_SRC_MEASURED, 5);
    rx(&l, p, n, t + LINK_TURN_WAIT_MS + 20);
    CHECK(got->n_turns == 5, "a shape after the time limit still arrives");
}

int main(void) {
    test_handshake();
    test_version();
    test_exchange();
    test_cancel();
    test_silence();
    test_errors();
    test_reconnect_and_status();
    test_live_data();
    test_turn_shape();
    test_turn_report_sync();
    if (s_fail) return EXIT_FAILURE;
    printf("link core tests: PASS\n");
    return EXIT_SUCCESS;
}
