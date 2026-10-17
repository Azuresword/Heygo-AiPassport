// main/audio/voice.h
// The voice task: microphone to ADPCM blocks, and ADPCM blocks to the speaker.
//
// One task owns the codec. It records or plays, never both. Capture reads one
// 20 ms block at a time, so a stop takes effect within one block.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*up_block)(const uint8_t *block);   // One 164-byte ADPCM block. Runs on the voice task.
    void (*play_done)(void);                  // The answer stream ended, or stalled for 2 s.
} voice_cb_t;

esp_err_t voice_start(const voice_cb_t *cb);

void voice_capture_start(void);
// Block until the task sends the last block. Returns false after 200 ms without an answer.
bool voice_capture_stop(void);

// The answer stream. The link calls these. The blocks wait until voice_play_enable().
void voice_down_open(void);
void voice_down_push(const uint8_t *block, size_t len);
void voice_down_end(void);

void voice_play_enable(void);
// Block until the speaker stops. Returns false after 200 ms without an answer.
bool voice_play_stop(void);

#ifdef __cplusplus
}
#endif
