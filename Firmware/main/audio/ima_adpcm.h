// main/audio/ima_adpcm.h
// IMA ADPCM with the block layout in doc 07, section 7.
//
//   offset 0, 2 bytes: predictor, signed 16-bit, little-endian
//   offset 2, 1 byte:  step index, 0 to 88
//   offset 3, 1 byte:  reserved, 0
//   offset 4, 160 bytes: 320 samples, 4 bits each, low nibble first
//
// The header holds the decoder state before the first sample, so a block
// decodes on its own. This layout differs from the WAV IMA ADPCM block,
// which stores the first sample in the header.
// Plain C. No ESP-IDF and no LVGL, so host tests can link it.
#pragma once

#include <stddef.h>
#include <stdint.h>

#define IMA_BLOCK_SAMPLES 320   // 20 ms at 16 kHz
#define IMA_BLOCK_BYTES   164
#define IMA_STEP_COUNT    89

typedef struct {
    int16_t predictor;
    uint8_t index;
} ima_state_t;

void ima_state_init(ima_state_t *s);

// Encodes `n` samples, with n from 0 to 320. Fills the rest of the block with
// samples of value 0, as doc 07 requires for the last block of a stream.
// Returns IMA_BLOCK_BYTES, or 0 for an invalid argument.
size_t ima_encode_block(ima_state_t *s, const int16_t *pcm, size_t n, uint8_t out[IMA_BLOCK_BYTES]);

// Decodes one block. Returns IMA_BLOCK_SAMPLES, or 0 for an invalid header.
size_t ima_decode_block(const uint8_t in[IMA_BLOCK_BYTES], int16_t pcm[IMA_BLOCK_SAMPLES]);
