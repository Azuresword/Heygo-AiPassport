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

static void test_no_rewait_and_silence_on_underrun(void) {
    play_gate_t g; play_gate_reset(&g);
    for (int i = 0; i < 3; i++) play_gate_on_block(&g, (uint32_t)i * 20);
    assert(play_gate_poll(&g, 40) == PLAY_GATE_PLAY);
    for (int i = 0; i < 3; i++) play_gate_on_played(&g);
    assert(play_gate_poll(&g, 100) == PLAY_GATE_SILENCE);
    play_gate_on_block(&g, 120);              // one block, not three
    assert(play_gate_poll(&g, 120) == PLAY_GATE_PLAY);
}

static void test_stall_ends_playback(void) {
    play_gate_t g; play_gate_reset(&g);
    for (int i = 0; i < 3; i++) play_gate_on_block(&g, 0);
    assert(play_gate_poll(&g, 0) == PLAY_GATE_PLAY);
    for (int i = 0; i < 3; i++) play_gate_on_played(&g);
    assert(play_gate_poll(&g, 1999) == PLAY_GATE_SILENCE);
    assert(play_gate_poll(&g, 2000) == PLAY_GATE_DONE);
}

static void test_clock_wrap(void) {
    play_gate_t g; play_gate_reset(&g);
    play_gate_on_block(&g, 0xFFFFFFF0u);
    assert(play_gate_poll(&g, 0xFFFFFFF0u + 59u) == PLAY_GATE_WAIT);
    assert(play_gate_poll(&g, 0xFFFFFFF0u + 60u) == PLAY_GATE_PLAY);
}

int main(void) {
    test_three_blocks_start();
    test_play_end_starts_with_fewer_blocks();
    test_play_end_with_no_block_plays_nothing();
    test_sixty_ms_after_first_block();
    test_no_rewait_and_silence_on_underrun();
    test_stall_ends_playback();
    test_clock_wrap();
    printf("play gate tests: PASS\n");
    return 0;
}
