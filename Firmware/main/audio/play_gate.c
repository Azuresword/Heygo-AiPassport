// main/audio/play_gate.c
#include "play_gate.h"

#include <string.h>

void play_gate_reset(play_gate_t *gate) {
    if (gate) memset(gate, 0, sizeof(*gate));
}

void play_gate_on_block(play_gate_t *gate, uint32_t now_ms) {
    if (!gate || gate->ended) return;
    if (!gate->have_first) {
        gate->have_first = true;
        gate->first_ms = now_ms;
    }
    if (gate->queued < UINT8_MAX) gate->queued++;
    gate->last_rx_ms = now_ms;
    gate->empty = false;
}

void play_gate_on_end(play_gate_t *gate, uint32_t now_ms) {
    if (!gate) return;
    gate->ended = true;
    gate->last_rx_ms = now_ms;
}

void play_gate_on_played(play_gate_t *gate) {
    if (gate && gate->queued > 0) gate->queued--;
}

void play_gate_on_written(play_gate_t *gate) {
    if (gate && gate->written < PLAY_GATE_GRACE_WRITES) gate->written++;
}

play_gate_action_t play_gate_poll(play_gate_t *gate, uint32_t now_ms) {
    if (!gate) return PLAY_GATE_DONE;

    if (!gate->started) {
        // PLAY_END with an empty queue: play nothing, and free the owner.
        if (gate->ended && gate->queued == 0) return PLAY_GATE_DONE;

        bool start = gate->queued >= PLAY_GATE_START_BLOCKS
                  || gate->ended
                  || (gate->have_first && (uint32_t)(now_ms - gate->first_ms) >= PLAY_GATE_START_MS);
        if (!start) return PLAY_GATE_WAIT;
        gate->started = true;
    }

    // After the start, the gate never waits for three blocks again.
    if (gate->queued > 0) return PLAY_GATE_PLAY;
    if (gate->ended) return PLAY_GATE_DONE;
    if ((uint32_t)(now_ms - gate->last_rx_ms) >= PLAY_GATE_STALL_MS) return PLAY_GATE_DONE;
    // Three 320-frame writes fill four 240-frame DMA descriptors.
    // Earlier waits can leave a partial descriptor behind the DMA write position.
    if (gate->written < PLAY_GATE_GRACE_WRITES) return PLAY_GATE_SILENCE;

    // I2S can accept the initial blocks before their sound reaches the speaker.
    // Give the next packet one block period before inserting silence.
    if (!gate->empty) {
        gate->empty = true;
        gate->empty_ms = now_ms;
    }
    if ((uint32_t)(now_ms - gate->empty_ms) < PLAY_GATE_GRACE_MS) return PLAY_GATE_WAIT;
    return PLAY_GATE_SILENCE;
}
