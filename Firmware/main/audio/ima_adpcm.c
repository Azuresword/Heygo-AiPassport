// main/audio/ima_adpcm.c
#include "ima_adpcm.h"

#include <string.h>

static const int8_t INDEX_TABLE[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static const int16_t STEP_TABLE[IMA_STEP_COUNT] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

_Static_assert(sizeof STEP_TABLE / sizeof STEP_TABLE[0] == IMA_STEP_COUNT, "IMA step table needs 89 entries");

static int clamp_index(int i) { return i < 0 ? 0 : (i > IMA_STEP_COUNT - 1 ? IMA_STEP_COUNT - 1 : i); }
static int32_t clamp_sample(int32_t v) { return v < -32768 ? -32768 : (v > 32767 ? 32767 : v); }

// Applies one nibble to the state. The encoder and the decoder share this
// function, so both sides follow the same predictor.
static int16_t apply_nibble(ima_state_t *s, uint8_t nibble) {
    int32_t step = STEP_TABLE[s->index];
    int32_t diff = step >> 3;
    if (nibble & 4) diff += step;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 1) diff += step >> 2;
    int32_t pred = s->predictor + ((nibble & 8) ? -diff : diff);
    s->predictor = (int16_t)clamp_sample(pred);
    s->index = (uint8_t)clamp_index((int)s->index + INDEX_TABLE[nibble & 0x0F]);
    return s->predictor;
}

static uint8_t encode_sample(ima_state_t *s, int16_t sample) {
    int32_t step = STEP_TABLE[s->index];
    int32_t diff = (int32_t)sample - s->predictor;
    uint8_t nibble = 0;
    if (diff < 0) { nibble = 8; diff = -diff; }
    if (diff >= step) { nibble |= 4; diff -= step; }
    step >>= 1;
    if (diff >= step) { nibble |= 2; diff -= step; }
    step >>= 1;
    if (diff >= step) { nibble |= 1; }
    apply_nibble(s, nibble);
    return nibble;
}

void ima_state_init(ima_state_t *s) {
    if (s) { s->predictor = 0; s->index = 0; }
}

size_t ima_encode_block(ima_state_t *s, const int16_t *pcm, size_t n, uint8_t out[IMA_BLOCK_BYTES]) {
    if (!s || !out || n > IMA_BLOCK_SAMPLES || (n > 0 && !pcm)) return 0;
    out[0] = (uint8_t)(uint16_t)s->predictor;
    out[1] = (uint8_t)((uint16_t)s->predictor >> 8);
    out[2] = s->index;
    out[3] = 0;
    memset(out + 4, 0, IMA_BLOCK_BYTES - 4);
    for (size_t k = 0; k < IMA_BLOCK_SAMPLES; k++) {
        int16_t sample = (k < n) ? pcm[k] : 0;     // Fill with samples of value 0.
        uint8_t nibble = encode_sample(s, sample);
        out[4 + k / 2] |= (k & 1) ? (uint8_t)(nibble << 4) : nibble;
    }
    return IMA_BLOCK_BYTES;
}

size_t ima_decode_block(const uint8_t in[IMA_BLOCK_BYTES], int16_t pcm[IMA_BLOCK_SAMPLES]) {
    if (!in || !pcm || in[2] > IMA_STEP_COUNT - 1) return 0;
    ima_state_t s;
    s.predictor = (int16_t)(uint16_t)(in[0] | (in[1] << 8));
    s.index = in[2];
    for (size_t k = 0; k < IMA_BLOCK_SAMPLES; k++) {
        uint8_t byte = in[4 + k / 2];
        uint8_t nibble = (k & 1) ? (uint8_t)(byte >> 4) : (uint8_t)(byte & 0x0F);
        pcm[k] = apply_nibble(&s, nibble);
    }
    return IMA_BLOCK_SAMPLES;
}
