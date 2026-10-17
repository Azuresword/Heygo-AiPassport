// Smooth the transition from silence to decoded PCM. No ESP-IDF dependency.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// 64 samples span 4 ms at the application's 16 kHz sample rate.
#define PLAY_ENVELOPE_FADE_SAMPLES 64

typedef struct {
    bool fade_pending;
} play_envelope_t;

void play_envelope_reset(play_envelope_t *envelope);
// False valid_pcm fills the block with zeros and arms the next fade.
// True valid_pcm applies one fade when armed. Later samples stay unchanged.
// Call only for a block that will be written. WAIT must not change this state.
// Return true if this call applies a fade.
bool play_envelope_apply(play_envelope_t *envelope, int16_t *pcm,
                         size_t samples, bool valid_pcm);
