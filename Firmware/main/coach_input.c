// main/coach_input.c
#include "coach_input.h"

#include <string.h>

static const coach_event_t HOLD[COACH_KEY_COUNT] = {
    [COACH_KEY_UP] = COACH_EVENT_UP_HOLD,
    [COACH_KEY_DOWN] = COACH_EVENT_DOWN_HOLD,
    [COACH_KEY_OK] = COACH_EVENT_OK_HOLD,
};

static const coach_event_t SHORT[COACH_KEY_COUNT] = {
    [COACH_KEY_UP] = COACH_EVENT_UP_SHORT,
    [COACH_KEY_DOWN] = COACH_EVENT_DOWN_SHORT,
    [COACH_KEY_OK] = COACH_EVENT_OK_SHORT,
};

void coach_input_init(coach_input_t *in) {
    if (in) memset(in, 0, sizeof(*in));
}

bool coach_input_map(coach_input_t *in, coach_key_t key, coach_key_ev_t ev, coach_event_t *out) {
    if (!in || !out || (unsigned)key >= COACH_KEY_COUNT) return false;
    switch (ev) {
    case COACH_KEY_EV_PRESS:
        in->held[key] = false;
        return false;
    case COACH_KEY_EV_LONG:
        in->held[key] = true;
        *out = HOLD[key];
        return true;
    case COACH_KEY_EV_RELEASE:
        if (!in->held[key]) return false;
        in->held[key] = false;
        if (key != COACH_KEY_OK) return false;
        *out = COACH_EVENT_OK_RELEASE;
        return true;
    case COACH_KEY_EV_CLICK:
    case COACH_KEY_EV_DOUBLE:
        // The component sends no CLICK after LONG. This check keeps that rule here too.
        if (in->held[key]) return false;
        *out = SHORT[key];
        return true;
    default:
        return false;
    }
}
