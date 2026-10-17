// Host tests for the playback start rules in doc 07, section 7.
#include "audio/play_gate.h"

#include <assert.h>
#include <stdio.h>

static void test_three_blocks_start(void) {
    play_gate_t g; play_gate_reset(&g);
    play_gate_on_block(&g, 0);
    play_gate_on_block(&g, 20);
    assert(play_gate_poll(&g, 40) == PLAY_GATE_WAIT);
    play_gate_on_block(&g, 40);
    assert(play_gate_poll(&g, 40) == PLAY_GATE_PLAY);
}

static void test_play_end_starts_with_fewer_blocks(void) {
    play_gate_t g; play_gate_reset(&g);
    play_gate_on_block(&g, 0);
    play_gate_on_end(&g, 5);
    assert(play_gate_poll(&g, 5) == PLAY_GATE_PLAY);
    play_gate_on_played(&g);
    assert(play_gate_poll(&g, 25) == PLAY_GATE_DONE);
}

static void test_play_end_with_no_block_plays_nothing(void) {
    play_gate_t g; play_gate_reset(&g);
    play_gate_on_end(&g, 0);
    assert(play_gate_poll(&g, 0) == PLAY_GATE_DONE);
}

static void test_sixty_ms_after_first_block(void) {
    play_gate_t g; play_gate_reset(&g);
    play_gate_on_block(&g, 1000);
    assert(play_gate_poll(&g, 1059) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 1060) == PLAY_GATE_PLAY);
}

static void start_and_drain(play_gate_t *g, uint32_t now_ms) {
    play_gate_reset(g);
    for (int i = 0; i < 3; i++) play_gate_on_block(g, now_ms);
    for (int i = 0; i < 3; i++) {
        assert(play_gate_poll(g, now_ms) == PLAY_GATE_PLAY);
        play_gate_on_played(g);
        play_gate_on_written(g);
    }
}

static void test_steady_twenty_ms_arrivals(void) {
    play_gate_t g; play_gate_reset(&g);
    for (uint32_t t = 0; t <= 40; t += 20) play_gate_on_block(&g, t);
    // The initial I2S writes can consume all three queued blocks immediately.
    for (int i = 0; i < 3; i++) {
        assert(play_gate_poll(&g, 40) == PLAY_GATE_PLAY);
        play_gate_on_played(&g);
        play_gate_on_written(&g);
    }
    for (uint32_t arrival = 60; arrival <= 6000; arrival += 20) {
        for (uint32_t t = arrival - 20; t < arrival; t += 5) {
            assert(play_gate_poll(&g, t) == PLAY_GATE_WAIT);
        }
        play_gate_on_block(&g, arrival);
        assert(play_gate_poll(&g, arrival) == PLAY_GATE_PLAY);
        play_gate_on_played(&g);
        play_gate_on_written(&g);
    }
    play_gate_on_end(&g, 6001);
    assert(play_gate_poll(&g, 6001) == PLAY_GATE_DONE);
}

static void test_grace_expires_once_at_time_zero(void) {
    play_gate_t g; start_and_drain(&g, 0);
    assert(play_gate_poll(&g, 0) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 19) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 20) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, 25) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, 40) == PLAY_GATE_SILENCE);
}

static void test_short_start_primes_before_grace(void) {
    for (unsigned available = 1; available <= 2; available++) {
        play_gate_t g; play_gate_reset(&g);
        for (unsigned i = 0; i < available; i++) play_gate_on_block(&g, i * 20);
        assert(play_gate_poll(&g, 59) == PLAY_GATE_WAIT);
        for (unsigned i = 0; i < available; i++) {
            assert(play_gate_poll(&g, 60) == PLAY_GATE_PLAY);
            play_gate_on_played(&g);
            play_gate_on_written(&g);
        }
        // A short start fills the remaining DMA space with explicit silence.
        for (unsigned i = available; i < 3; i++) {
            assert(play_gate_poll(&g, 60) == PLAY_GATE_SILENCE);
            play_gate_on_written(&g);
        }
        assert(play_gate_poll(&g, 60) == PLAY_GATE_WAIT);
        assert(play_gate_poll(&g, 79) == PLAY_GATE_WAIT);
        assert(play_gate_poll(&g, 80) == PLAY_GATE_SILENCE);
    }
}

static void test_removed_blocks_do_not_arm_grace(void) {
    play_gate_t g; play_gate_reset(&g);
    for (int i = 0; i < 3; i++) play_gate_on_block(&g, 0);
    assert(play_gate_poll(&g, 0) == PLAY_GATE_PLAY);
    // Queue removal also handles dropped blocks. It does not confirm I2S writes.
    for (int i = 0; i < 3; i++) play_gate_on_played(&g);
    assert(play_gate_poll(&g, 0) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, 20) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, 2000) == PLAY_GATE_DONE);
}

static void test_unconfirmed_write_does_not_arm_grace(void) {
    play_gate_t g; play_gate_reset(&g);
    for (int i = 0; i < 3; i++) play_gate_on_block(&g, 0);
    for (int i = 0; i < 3; i++) {
        assert(play_gate_poll(&g, 0) == PLAY_GATE_PLAY);
        play_gate_on_played(&g);
        // The caller reports success for only the first two writes.
        if (i < 2) play_gate_on_written(&g);
    }
    assert(play_gate_poll(&g, 0) == PLAY_GATE_SILENCE);
    play_gate_on_written(&g);
    assert(play_gate_poll(&g, 0) == PLAY_GATE_WAIT);
}

static void test_short_start_end_bypasses_priming(void) {
    play_gate_t g; play_gate_reset(&g);
    play_gate_on_block(&g, 0);
    assert(play_gate_poll(&g, 60) == PLAY_GATE_PLAY);
    play_gate_on_played(&g);
    play_gate_on_written(&g);
    play_gate_on_end(&g, 61);
    assert(play_gate_poll(&g, 61) == PLAY_GATE_DONE);
}

static void test_one_block_resumes_and_resets_grace(void) {
    play_gate_t g; start_and_drain(&g, 1000);
    assert(play_gate_poll(&g, 1000) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 1020) == PLAY_GATE_SILENCE);
    play_gate_on_block(&g, 1030);
    assert(play_gate_poll(&g, 1030) == PLAY_GATE_PLAY);
    play_gate_on_played(&g);
    play_gate_on_written(&g);
    assert(play_gate_poll(&g, 1030) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 1049) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 1050) == PLAY_GATE_SILENCE);
}

static void test_play_end_bypasses_grace(void) {
    play_gate_t g; start_and_drain(&g, 1000);
    assert(play_gate_poll(&g, 1000) == PLAY_GATE_WAIT);
    play_gate_on_end(&g, 1005);
    assert(play_gate_poll(&g, 1005) == PLAY_GATE_DONE);
}

static void test_play_end_drains_resumed_block(void) {
    play_gate_t g; start_and_drain(&g, 1000);
    assert(play_gate_poll(&g, 1000) == PLAY_GATE_WAIT);
    play_gate_on_block(&g, 1005);
    play_gate_on_end(&g, 1006);
    assert(play_gate_poll(&g, 1006) == PLAY_GATE_PLAY);
    play_gate_on_played(&g);
    play_gate_on_written(&g);
    assert(play_gate_poll(&g, 1006) == PLAY_GATE_DONE);
}

static void test_stall_ends_playback(void) {
    play_gate_t g; start_and_drain(&g, 0);
    assert(play_gate_poll(&g, 0) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 20) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, 1999) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, 2000) == PLAY_GATE_DONE);
}

static void test_stall_bypasses_new_grace(void) {
    play_gate_t g; start_and_drain(&g, 0);
    assert(play_gate_poll(&g, 1999) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 2000) == PLAY_GATE_DONE);
}

static void test_reset_clears_grace(void) {
    play_gate_t g; start_and_drain(&g, 0);
    assert(play_gate_poll(&g, 0) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 20) == PLAY_GATE_SILENCE);
    play_gate_reset(&g);
    assert(play_gate_poll(&g, 100) == PLAY_GATE_WAIT);
    play_gate_on_block(&g, 100);
    assert(play_gate_poll(&g, 159) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 160) == PLAY_GATE_PLAY);
    play_gate_on_played(&g);
    play_gate_on_written(&g);
    for (int i = 0; i < 2; i++) {
        assert(play_gate_poll(&g, 160) == PLAY_GATE_SILENCE);
        play_gate_on_written(&g);
    }
    assert(play_gate_poll(&g, 160) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 179) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 180) == PLAY_GATE_SILENCE);
}

static void test_clock_wrap(void) {
    play_gate_t g; play_gate_reset(&g);
    play_gate_on_block(&g, 0xFFFFFFF0u);
    assert(play_gate_poll(&g, 0xFFFFFFF0u + 59u) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 0xFFFFFFF0u + 60u) == PLAY_GATE_PLAY);
}

static void test_grace_clock_wrap(void) {
    const uint32_t t = UINT32_MAX - 9u;
    play_gate_t g; start_and_drain(&g, t);
    assert(play_gate_poll(&g, t) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, t + 19u) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, t + 20u) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, t + 1999u) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, t + 2000u) == PLAY_GATE_DONE);
}

int main(void) {
    test_three_blocks_start();
    test_play_end_starts_with_fewer_blocks();
    test_play_end_with_no_block_plays_nothing();
    test_sixty_ms_after_first_block();
    test_steady_twenty_ms_arrivals();
    test_grace_expires_once_at_time_zero();
    test_short_start_primes_before_grace();
    test_removed_blocks_do_not_arm_grace();
    test_unconfirmed_write_does_not_arm_grace();
    test_short_start_end_bypasses_priming();
    test_one_block_resumes_and_resets_grace();
    test_play_end_bypasses_grace();
    test_play_end_drains_resumed_block();
    test_stall_ends_playback();
    test_stall_bypasses_new_grace();
    test_reset_clears_grace();
    test_clock_wrap();
    test_grace_clock_wrap();
    printf("play gate tests: PASS\n");
    return 0;
}
