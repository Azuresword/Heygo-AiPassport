#include "play_envelope.h"

#include <string.h>

void play_envelope_reset(play_envelope_t *envelope) {
    envelope->fade_pending = true;
}

bool play_envelope_apply(play_envelope_t *envelope, int16_t *pcm,
                         size_t samples, bool valid_pcm) {
    if (samples == 0) return false;
    if (!valid_pcm) {
        memset(pcm, 0, samples * sizeof(*pcm));
        envelope->fade_pending = true;
        return false;
    }
    if (!envelope->fade_pending) return false;

    size_t count = samples < PLAY_ENVELOPE_FADE_SAMPLES
                 ? samples : PLAY_ENVELOPE_FADE_SAMPLES;
    // Short blocks use a shorter ramp. A one-sample block starts at zero.
    const int32_t divisor = count > 1 ? (int32_t)(count - 1) : 1;
    for (size_t i = 0; i < count; i++) {
        pcm[i] = (int16_t)((int32_t)pcm[i] * (int32_t)i / divisor);
    }
    envelope->fade_pending = false;
    return true;
}
