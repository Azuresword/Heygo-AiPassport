// Host tests for the IMA ADPCM codec in Sprint 0, task T-3.
#include "audio/ima_adpcm.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define BLOCKS 50   // one second at 16 kHz

static int16_t pcm_in[BLOCKS][IMA_BLOCK_SAMPLES];
static uint8_t enc[BLOCKS][IMA_BLOCK_BYTES];
static int16_t pcm_out[BLOCKS][IMA_BLOCK_SAMPLES];

static void make_sine(double amplitude) {
    for (int b = 0; b < BLOCKS; b++)
        for (int k = 0; k < IMA_BLOCK_SAMPLES; k++) {
            double t = (double)(b * IMA_BLOCK_SAMPLES + k) / 16000.0;
            pcm_in[b][k] = (int16_t)lrint(amplitude * sin(2.0 * M_PI * 1000.0 * t));
        }
}

static void encode_all(void) {
    ima_state_t s; ima_state_init(&s);
    for (int b = 0; b < BLOCKS; b++) assert(ima_encode_block(&s, pcm_in[b], IMA_BLOCK_SAMPLES, enc[b]) == IMA_BLOCK_BYTES);
}

static void test_block_size_and_header(void) {
    make_sine(10000.0);
    encode_all();
    // Block 0 starts from the initial state: predictor 0, index 0.
    assert(enc[0][0] == 0 && enc[0][1] == 0 && enc[0][2] == 0 && enc[0][3] == 0);
    for (int b = 0; b < BLOCKS; b++) assert(enc[b][2] <= 88 && enc[b][3] == 0);
}

static double snr_db(void) {
    double sig = 0, err = 0;
    for (int b = 1; b < BLOCKS; b++)              // skip block 0: the step size adapts there
        for (int k = 0; k < IMA_BLOCK_SAMPLES; k++) {
            double x = pcm_in[b][k], e = x - pcm_out[b][k];
            sig += x * x; err += e * e;
        }
    return 10.0 * log10(sig / (err > 0 ? err : 1e-9));
}

static void test_sine_round_trip(void) {
    make_sine(10000.0);
    encode_all();
    for (int b = 0; b < BLOCKS; b++) assert(ima_decode_block(enc[b], pcm_out[b]) == IMA_BLOCK_SAMPLES);
    double snr = snr_db();
    printf("  1 kHz sine SNR: %.1f dB\n", snr);
    assert(snr > 20.0);
}

static void test_lost_block(void) {
    make_sine(10000.0);
    encode_all();
    int16_t in_order[IMA_BLOCK_SAMPLES], after_loss[IMA_BLOCK_SAMPLES], scratch[IMA_BLOCK_SAMPLES];
    ima_decode_block(enc[9], scratch);
    ima_decode_block(enc[10], scratch);
    assert(ima_decode_block(enc[11], in_order) == IMA_BLOCK_SAMPLES);
    // Block 10 is lost. Block 11 still decodes to the same samples.
    ima_decode_block(enc[9], scratch);
    assert(ima_decode_block(enc[11], after_loss) == IMA_BLOCK_SAMPLES);
    assert(memcmp(in_order, after_loss, sizeof in_order) == 0);
}

static void test_full_scale(void) {
    ima_state_t s; ima_state_init(&s);
    int16_t square[IMA_BLOCK_SAMPLES];
    for (int k = 0; k < IMA_BLOCK_SAMPLES; k++) square[k] = (k / 8) % 2 ? 32767 : -32768;
    for (int b = 0; b < 20; b++) {
        uint8_t blk[IMA_BLOCK_BYTES]; int16_t out[IMA_BLOCK_SAMPLES];
        assert(ima_encode_block(&s, square, IMA_BLOCK_SAMPLES, blk) == IMA_BLOCK_BYTES);
        assert(blk[2] <= 88);
        assert(ima_decode_block(blk, out) == IMA_BLOCK_SAMPLES);
    }
    make_sine(32767.0);
    encode_all();
    for (int b = 0; b < BLOCKS; b++) assert(ima_decode_block(enc[b], pcm_out[b]) == IMA_BLOCK_SAMPLES);
}

static void test_short_last_block_fill(void) {
    make_sine(10000.0);
    int16_t filled[IMA_BLOCK_SAMPLES] = {0};
    memcpy(filled, pcm_in[3], 100 * sizeof(int16_t));
    ima_state_t a, b; ima_state_init(&a); ima_state_init(&b);
    uint8_t short_blk[IMA_BLOCK_BYTES], full_blk[IMA_BLOCK_BYTES];
    assert(ima_encode_block(&a, pcm_in[3], 100, short_blk) == IMA_BLOCK_BYTES);
    assert(ima_encode_block(&b, filled, IMA_BLOCK_SAMPLES, full_blk) == IMA_BLOCK_BYTES);
    assert(memcmp(short_blk, full_blk, IMA_BLOCK_BYTES) == 0);   // fill equals samples of value 0
    assert(ima_encode_block(&a, NULL, 0, short_blk) == IMA_BLOCK_BYTES);   // an empty last block
}

static void test_invalid_input(void) {
    uint8_t blk[IMA_BLOCK_BYTES] = {0};
    int16_t out[IMA_BLOCK_SAMPLES];
    blk[2] = 89;
    assert(ima_decode_block(blk, out) == 0);
    ima_state_t s; ima_state_init(&s);
    int16_t pcm[IMA_BLOCK_SAMPLES] = {0};
    assert(ima_encode_block(&s, pcm, IMA_BLOCK_SAMPLES + 1, blk) == 0);
}

int main(void) {
    test_block_size_and_header();
    test_sine_round_trip();
    test_lost_block();
    test_full_scale();
    test_short_last_block_fill();
    test_invalid_input();
    printf("ima adpcm tests: PASS\n");
    return 0;
}
