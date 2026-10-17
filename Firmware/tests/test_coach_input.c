// tests/test_coach_input.c
// Host test for the button-to-flow map, plus the full Coach flow driven by raw key events.
#include <stdio.h>
#include <stdlib.h>

#include "coach_flow.h"
#include "coach_input.h"

static int s_fail;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); s_fail++; } } while (0)

static coach_input_t s_in;
static coach_flow_t s_flow;

// Send one raw key event. Return true when the flow handled the mapped event.
static bool key(coach_key_t k, coach_key_ev_t ev) {
    coach_event_t e;
    if (!coach_input_map(&s_in, k, ev, &e)) return false;
    return coach_flow_step(&s_flow, e).handled;
}

static void short_press(coach_key_t k) {
    key(k, COACH_KEY_EV_PRESS); key(k, COACH_KEY_EV_RELEASE); key(k, COACH_KEY_EV_CLICK);
}
static void hold_press(coach_key_t k) {
    key(k, COACH_KEY_EV_PRESS); key(k, COACH_KEY_EV_LONG); key(k, COACH_KEY_EV_RELEASE);
}

static void test_map(void) {
    coach_event_t e;
    coach_input_init(&s_in);
    CHECK(!coach_input_map(&s_in, COACH_KEY_OK, COACH_KEY_EV_PRESS, &e), "press gives nothing");
    CHECK(coach_input_map(&s_in, COACH_KEY_OK, COACH_KEY_EV_LONG, &e) && e == COACH_EVENT_OK_HOLD, "long gives OK_HOLD");
    CHECK(!coach_input_map(&s_in, COACH_KEY_OK, COACH_KEY_EV_CLICK, &e), "no short after a hold");
    CHECK(coach_input_map(&s_in, COACH_KEY_OK, COACH_KEY_EV_RELEASE, &e) && e == COACH_EVENT_OK_RELEASE, "release after hold");
    CHECK(!coach_input_map(&s_in, COACH_KEY_OK, COACH_KEY_EV_RELEASE, &e), "second release gives nothing");

    CHECK(!coach_input_map(&s_in, COACH_KEY_UP, COACH_KEY_EV_RELEASE, &e), "short release gives nothing");
    CHECK(coach_input_map(&s_in, COACH_KEY_UP, COACH_KEY_EV_CLICK, &e) && e == COACH_EVENT_UP_SHORT, "click gives UP_SHORT");
    CHECK(coach_input_map(&s_in, COACH_KEY_DOWN, COACH_KEY_EV_DOUBLE, &e) && e == COACH_EVENT_DOWN_SHORT, "double gives one short");
    CHECK(coach_input_map(&s_in, COACH_KEY_DOWN, COACH_KEY_EV_LONG, &e) && e == COACH_EVENT_DOWN_HOLD, "long gives DOWN_HOLD");
    CHECK(!coach_input_map(&s_in, COACH_KEY_DOWN, COACH_KEY_EV_RELEASE, &e), "DOWN release gives nothing");
    CHECK(!coach_input_map(&s_in, (coach_key_t)7, COACH_KEY_EV_LONG, &e), "bad key");
    CHECK(!coach_input_map(NULL, COACH_KEY_OK, COACH_KEY_EV_LONG, &e), "null map");
}

static void test_flow_by_keys(void) {
    coach_input_init(&s_in);
    coach_flow_init(&s_flow);
    CHECK(s_flow.state == COACH_STATE_SKI, "starts in SKI");

    hold_press(COACH_KEY_OK);
    CHECK(s_flow.state == COACH_STATE_READY, "the hold enters READY and its release starts nothing");

    key(COACH_KEY_OK, COACH_KEY_EV_PRESS);
    key(COACH_KEY_OK, COACH_KEY_EV_LONG);
    CHECK(s_flow.state == COACH_STATE_CAPTURE, "a second hold starts capture");
    key(COACH_KEY_OK, COACH_KEY_EV_RELEASE);
    CHECK(s_flow.state == COACH_STATE_WAIT_ANSWER, "release ends the question");

    hold_press(COACH_KEY_UP);
    CHECK(s_flow.state == COACH_STATE_READY, "an UP hold cancels");

    short_press(COACH_KEY_OK);
    CHECK(s_flow.state == COACH_STATE_READY, "a short OK in READY does nothing");

    hold_press(COACH_KEY_DOWN);
    CHECK(s_flow.state == COACH_STATE_EXIT_Q, "a DOWN hold opens the exit question");
    short_press(COACH_KEY_OK);
    CHECK(s_flow.state == COACH_STATE_SKI, "a short OK returns to SKI");
    CHECK(coach_state_screen(s_flow.state) == COACH_SCREEN_SKI, "SKI uses the ski screen");
    CHECK(coach_state_screen(COACH_STATE_CAPTURE) == COACH_SCREEN_COACH, "CAPTURE uses the coach screen");
    CHECK(coach_state_screen(COACH_STATE_EXIT_Q) == COACH_SCREEN_EXIT, "EXIT_Q uses the exit screen");
}

int main(void) {
    test_map();
    test_flow_by_keys();
    if (s_fail) return EXIT_FAILURE;
    printf("coach input tests: PASS\n");
    return EXIT_SUCCESS;
}
