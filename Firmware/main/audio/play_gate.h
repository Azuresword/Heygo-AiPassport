// main/audio/play_gate.h
// Decides when the speaker starts and stops for one answer stream.
// The rules come from doc 07, section 7, "Playback start".
// Plain C. No ESP-IDF and no LVGL, so host tests can link it.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PLAY_GATE_START_BLOCKS 3      // Start when three blocks wait.
#define PLAY_GATE_START_MS     60     // Or 60 ms after the first block.
#define PLAY_GATE_STALL_MS     2000   // End after 2 s with no block and no PLAY_END.

typedef enum {
    PLAY_GATE_WAIT = 0,   // Keep the speaker off. Wait for more blocks.
    PLAY_GATE_PLAY,       // Play the next block from the queue.
    PLAY_GATE_SILENCE,    // The queue is empty. Play silence for one block.
    PLAY_GATE_DONE,       // The stream ended. Free the audio owner.
} play_gate_action_t;

typedef struct {
    uint8_t queued;       // Blocks that wait in the playback queue.
    bool started;
    bool ended;           // PLAY_END arrived.
    bool have_first;
    uint32_t first_ms;    // Arrival time of the first block.
    uint32_t last_rx_ms;  // Arrival time of the latest block or PLAY_END.
} play_gate_t;

void play_gate_reset(play_gate_t *gate);
void play_gate_on_block(play_gate_t *gate, uint32_t now_ms);
void play_gate_on_end(play_gate_t *gate, uint32_t now_ms);
// Call once for each block that the speaker consumes.
void play_gate_on_played(play_gate_t *gate);
play_gate_action_t play_gate_poll(play_gate_t *gate, uint32_t now_ms);
