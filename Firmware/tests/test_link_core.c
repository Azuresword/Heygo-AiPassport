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

static const link_io_t IO = { NULL, io_send, io_drop, io_voice, io_state, io_play_start, io_down, io_play_end, io_dropped };

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
    ack(l, 0, 1, t + 10);
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
    CHECK(h && h->buf[4] == 0 && h->buf[5] == 1 && h->buf[6] == BP_CODEC_MASK_ADPCM, "HELLO carries 0.1 and ADPCM only");
    CHECK(h && h->buf[7] == 23 && h->buf[8] == 0, "HELLO carries the current MTU");
    CHECK(s_voice == LINK_VOICE_WAIT_HELLO, "waits for HELLO_ACK");
    link_tick(&l, 2999);
    CHECK(s_voice == LINK_VOICE_WAIT_HELLO, "still waits at 1999 ms");
    link_tick(&l, 3000);
    CHECK(s_voice == LINK_VOICE_NO_ACK, "no HELLO_ACK after 2 s");

    clear();
    ack(&l, 0, 1, 3100);
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
    ack(&l, 0, 2, 50);
    const sent_t *e = last();
    CHECK(e && e->buf[0] == BP_ERROR && e->buf[4] == BP_ERR_VERSION && e->buf[5] == BP_HELLO_ACK, "ERROR 3 for version 0.2");
    CHECK(s_voice == LINK_VOICE_VERSION, "a version mismatch blocks voice");
    CHECK(link_agent_start(&l) == 0, "no exchange without voice");
    ack(&l, 0, 1, 60);
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
    ack(&l, 0, 1, t + 3);
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

int main(void) {
    test_handshake();
    test_version();
    test_exchange();
    test_cancel();
    test_silence();
    test_errors();
    test_reconnect_and_status();
    if (s_fail) return EXIT_FAILURE;
    printf("link core tests: PASS\n");
    return EXIT_SUCCESS;
}
