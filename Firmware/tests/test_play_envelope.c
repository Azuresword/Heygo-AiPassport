#include "audio/play_envelope.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { BLOCK_SAMPLES = 320 };

static void fill(int16_t *pcm, int16_t value) {
    for (size_t i = 0; i < BLOCK_SAMPLES; i++) pcm[i] = value;
}

static void test_full_scale_ramp(int16_t value) {
    play_envelope_t env;
    play_envelope_reset(&env);
    int16_t pcm[BLOCK_SAMPLES];
    fill(pcm, value);
    assert(play_envelope_apply(&env, pcm, BLOCK_SAMPLES, true));
    assert(pcm[0] == 0);
    for (size_t i = 1; i < PLAY_ENVELOPE_FADE_SAMPLES; i++) {
        if (value >= 0) assert(pcm[i] >= pcm[i - 1] && pcm[i] <= value);
        else assert(pcm[i] <= pcm[i - 1] && pcm[i] >= value);
    }
    for (size_t i = PLAY_ENVELOPE_FADE_SAMPLES - 1; i < BLOCK_SAMPLES; i++) {
        assert(pcm[i] == value);
    }
}

static void test_continuity_and_silence(void) {
    play_envelope_t env;
    play_envelope_reset(&env);
    int16_t pcm[BLOCK_SAMPLES], original[BLOCK_SAMPLES];
    for (size_t i = 0; i < BLOCK_SAMPLES; i++) {
        original[i] = i % 2 ? -12345 : 23456;
    }
    memcpy(pcm, original, sizeof pcm);
    assert(play_envelope_apply(&env, pcm, BLOCK_SAMPLES, true));
    // Repeated valid blocks must not receive a periodic volume ramp.
    for (unsigned block = 0; block < 10; block++) {
        memcpy(pcm, original, sizeof pcm);
        assert(!play_envelope_apply(&env, pcm, BLOCK_SAMPLES, true));
        assert(memcmp(pcm, original, sizeof pcm) == 0);
    }
    // Missing or invalid data must produce exact silence, including repeats.
    for (unsigned block = 0; block < 3; block++) {
        memcpy(pcm, original, sizeof pcm);
        assert(!play_envelope_apply(&env, pcm, BLOCK_SAMPLES, false));
        for (size_t i = 0; i < BLOCK_SAMPLES; i++) assert(pcm[i] == 0);
    }
    memcpy(pcm, original, sizeof pcm);
    assert(play_envelope_apply(&env, pcm, BLOCK_SAMPLES, true));
    assert(pcm[0] == 0);
    for (size_t i = 0; i < BLOCK_SAMPLES; i++) {
        const int before = original[i] < 0 ? -original[i] : original[i];
        const int after = pcm[i] < 0 ? -pcm[i] : pcm[i];
        assert(after <= before);
        assert(pcm[i] == 0 || (pcm[i] < 0) == (original[i] < 0));
    }
    play_envelope_reset(&env);
    assert(play_envelope_apply(&env, pcm, BLOCK_SAMPLES, true));
}

static void test_short_and_empty_blocks(void) {
    play_envelope_t env;
    play_envelope_reset(&env);
    assert(!play_envelope_apply(&env, NULL, 0, true));
    int16_t pcm[] = {-32768, 32767, -123};
    assert(play_envelope_apply(&env, pcm, 2, true));
    assert(pcm[0] == 0 && pcm[1] == 32767 && pcm[2] == -123);
    play_envelope_reset(&env);
    assert(play_envelope_apply(&env, pcm, 1, true));
    assert(pcm[0] == 0 && pcm[1] == 32767);
}

int main(void) {
    test_full_scale_ramp(32767);
    test_full_scale_ramp(-32768);
    test_full_scale_ramp(0);
    test_continuity_and_silence();
    test_short_and_empty_blocks();
    puts("play envelope tests: PASS");
    return 0;
}
