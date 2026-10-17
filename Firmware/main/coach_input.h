// main/coach_input.h
// Maps raw button events to Coach flow events. Plain C, so host tests can link it.
//
// The button component sends these events for one key:
//   short press  PRESS, RELEASE, then CLICK about 180 ms after the release
//   double press PRESS, RELEASE, PRESS, RELEASE, then DOUBLE (no CLICK)
//   long press   PRESS, LONG at 500 ms, RELEASE (no CLICK)
//
// The map gives:
//   LONG               -> <KEY>_HOLD
//   RELEASE after LONG -> OK_RELEASE (the flow uses no UP or DOWN release)
//   CLICK or DOUBLE    -> <KEY>_SHORT, once
// A hold never also gives a short press.
#pragma once

#include <stdbool.h>

#include "coach_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

// The same order as bsp_btn_t.
typedef enum {
    COACH_KEY_UP = 0,
    COACH_KEY_DOWN,
    COACH_KEY_OK,
    COACH_KEY_COUNT
} coach_key_t;

// The same order as bsp_btn_ev_t.
typedef enum {
    COACH_KEY_EV_PRESS = 0,
    COACH_KEY_EV_CLICK,
    COACH_KEY_EV_DOUBLE,
    COACH_KEY_EV_LONG,
    COACH_KEY_EV_RELEASE,
} coach_key_ev_t;

typedef struct {
    bool held[COACH_KEY_COUNT];   // true after LONG, until the next PRESS or RELEASE
} coach_input_t;

void coach_input_init(coach_input_t *in);

// Return true and write *out when the key event gives a flow event.
bool coach_input_map(coach_input_t *in, coach_key_t key, coach_key_ev_t ev, coach_event_t *out);

#ifdef __cplusplus
}
#endif
