// Host tests for the P0 paths in Sprint 0, task T-1.
// The generated table proves that each transition fires. These tests prove
// the required paths and the audio rules across long event sequences.
#include "coach_flow.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef enum { OWNER_NONE, OWNER_CAPTURE, OWNER_PLAYBACK } owner_t;

static bool has_action(const coach_step_t *s, coach_action_t a) {
    for (size_t i = 0; i < s->action_count; i++) if (s->actions[i] == a) return true;
    return false;
}

static int index_of(const coach_step_t *s, coach_action_t a) {
    for (size_t i = 0; i < s->action_count; i++) if (s->actions[i] == a) return (int)i;
    return -1;
}

// Apply the actions of one step to a simulated audio owner.
// Assert that the device never captures and plays at the same time.
static owner_t apply(owner_t owner, const coach_step_t *s) {
    for (size_t i = 0; i < s->action_count; i++) {
        switch (s->actions[i]) {
        case COACH_ACTION_AUDIO_CAPTURE_START: assert(owner == OWNER_NONE); owner = OWNER_CAPTURE; break;
        case COACH_ACTION_AUDIO_PLAY_START:    assert(owner == OWNER_NONE); owner = OWNER_PLAYBACK; break;
        case COACH_ACTION_AUDIO_CAPTURE_STOP:  assert(owner == OWNER_CAPTURE); owner = OWNER_NONE; break;
        case COACH_ACTION_AUDIO_PLAY_STOP:     assert(owner == OWNER_PLAYBACK); owner = OWNER_NONE; break;
        default: break;
        }
    }
    return owner;
}

static coach_step_t step(coach_flow_t *f, coach_event_t e) { return coach_flow_step(f, e); }

// Path 1: the press that enters Rest mode never starts capture.
static void path_enter_rest(void) {
    coach_flow_t f; coach_flow_init(&f);
    assert(f.state == COACH_STATE_SKI);
    coach_step_t s = step(&f, COACH_EVENT_OK_HOLD);
    assert(f.state == COACH_STATE_READY);
    assert(!has_action(&s, COACH_ACTION_AUDIO_CAPTURE_START));
    s = step(&f, COACH_EVENT_OK_RELEASE);          // the same press ends
    assert(!s.handled && f.state == COACH_STATE_READY);
}

// Paths 2 and 3: a new hold starts capture. Release or the 20 s limit ends it.
static void path_capture_and_end(void) {
    coach_flow_t f; coach_flow_init(&f); f.state = COACH_STATE_READY;
    coach_step_t s = step(&f, COACH_EVENT_OK_HOLD);
    assert(f.state == COACH_STATE_CAPTURE);
    assert(has_action(&s, COACH_ACTION_AUDIO_CAPTURE_START));
    assert(has_action(&s, COACH_ACTION_LINK_SEND_AGENT_START));
    s = step(&f, COACH_EVENT_OK_RELEASE);
    assert(f.state == COACH_STATE_WAIT_ANSWER);
    assert(has_action(&s, COACH_ACTION_AUDIO_CAPTURE_STOP));
    assert(has_action(&s, COACH_ACTION_LINK_SEND_AGENT_END));

    f.state = COACH_STATE_CAPTURE;
    s = step(&f, COACH_EVENT_CAPTURE_LIMIT);
    assert(f.state == COACH_STATE_WAIT_ANSWER);
    assert(has_action(&s, COACH_ACTION_AUDIO_CAPTURE_STOP));
}

// Path 4: an UP hold cancels capture and cancels playback.
static void path_cancel(void) {
    coach_flow_t f; coach_flow_init(&f);
    f.state = COACH_STATE_CAPTURE;
    coach_step_t s = step(&f, COACH_EVENT_UP_HOLD);
    assert(f.state == COACH_STATE_READY);
    assert(has_action(&s, COACH_ACTION_AUDIO_CAPTURE_STOP));
    assert(has_action(&s, COACH_ACTION_LINK_SEND_AGENT_CANCEL));

    f.state = COACH_STATE_PLAYBACK;
    s = step(&f, COACH_EVENT_UP_HOLD);
    assert(f.state == COACH_STATE_READY);
    assert(has_action(&s, COACH_ACTION_AUDIO_PLAY_STOP));
    assert(has_action(&s, COACH_ACTION_LINK_SEND_AGENT_CANCEL));

    f.state = COACH_STATE_READY;                    // nothing runs: no action
    s = step(&f, COACH_EVENT_UP_HOLD);
    assert(!s.handled && f.state == COACH_STATE_READY);
}

// Path 5: an OK hold during playback stops playback before capture starts.
static void path_barge_in(void) {
    coach_flow_t f; coach_flow_init(&f); f.state = COACH_STATE_PLAYBACK;
    coach_step_t s = step(&f, COACH_EVENT_OK_HOLD);
    assert(f.state == COACH_STATE_CAPTURE);
    int stop = index_of(&s, COACH_ACTION_AUDIO_PLAY_STOP);
    int start = index_of(&s, COACH_ACTION_AUDIO_CAPTURE_START);
    assert(stop >= 0 && start >= 0 && stop < start);
}

// Path 6: a DOWN hold and a short OK return to Ski mode.
static void path_exit(void) {
    coach_flow_t f; coach_flow_init(&f); f.state = COACH_STATE_READY;
    step(&f, COACH_EVENT_DOWN_HOLD);
    assert(f.state == COACH_STATE_EXIT_Q);
    step(&f, COACH_EVENT_OK_SHORT);
    assert(f.state == COACH_STATE_SKI);

    f.state = COACH_STATE_EXIT_Q;                   // any other input closes it
    step(&f, COACH_EVENT_UP_SHORT);
    assert(f.state == COACH_STATE_READY);

    f.state = COACH_STATE_CAPTURE;                  // DOWN does nothing while audio runs
    coach_step_t s = step(&f, COACH_EVENT_DOWN_HOLD);
    assert(!s.handled && f.state == COACH_STATE_CAPTURE);
}

// Path 7: link loss stops capture and playback.
static void path_link_loss(void) {
    coach_flow_t f; coach_flow_init(&f);
    f.state = COACH_STATE_CAPTURE;
    coach_step_t s = step(&f, COACH_EVENT_LINK_LOST);
    assert(f.state == COACH_STATE_READY && has_action(&s, COACH_ACTION_AUDIO_CAPTURE_STOP));
    f.state = COACH_STATE_PLAYBACK;
    s = step(&f, COACH_EVENT_LINK_LOST);
    assert(f.state == COACH_STATE_READY && has_action(&s, COACH_ACTION_AUDIO_PLAY_STOP));
}

// Invariants over a long, deterministic random walk.
static void walk_invariants(void) {
    coach_flow_t f; coach_flow_init(&f);
    owner_t owner = OWNER_NONE;
    uint32_t seed = 12345u;
    for (int i = 0; i < 20000; i++) {
        seed = seed * 1103515245u + 12345u;
        coach_event_t e = (coach_event_t)((seed >> 16) % COACH_EVENT_COUNT);
        coach_step_t s = step(&f, e);
        owner = apply(owner, &s);
        if (f.state == COACH_STATE_SKI || f.state == COACH_STATE_READY ||
            f.state == COACH_STATE_EXIT_Q || f.state == COACH_STATE_WAIT_ANSWER) {
            assert(owner == OWNER_NONE);           // no audio outside capture and playback
        }
        if (f.state == COACH_STATE_CAPTURE) assert(owner == OWNER_CAPTURE);
        if (f.state == COACH_STATE_PLAYBACK) assert(owner == OWNER_PLAYBACK);
    }
}

int main(void) {
    path_enter_rest();
    path_capture_and_end();
    path_cancel();
    path_barge_in();
    path_exit();
    path_link_loss();
    walk_invariants();
    printf("coach path tests: PASS\n");
    return 0;
}
