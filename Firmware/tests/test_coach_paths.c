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

// Path 1 (D-29): an OK hold opens the Coach page from every page, and never starts capture there.
static void path_ok_hold_opens_coach(void) {
    const coach_state_t pages[] = { COACH_STATE_LIVE, COACH_STATE_REPORT, COACH_STATE_TURNS };
    for (size_t i = 0; i < sizeof pages / sizeof pages[0]; i++) {
        coach_flow_t f; coach_flow_init(&f); f.state = pages[i];
        coach_step_t s = step(&f, COACH_EVENT_OK_HOLD);
        assert(f.state == COACH_STATE_READY);
        assert(!has_action(&s, COACH_ACTION_AUDIO_CAPTURE_START));
        s = step(&f, COACH_EVENT_OK_RELEASE);      // the same press ends
        assert(!s.handled && f.state == COACH_STATE_READY);
    }
    coach_flow_t f; coach_flow_init(&f);
    assert(f.state == COACH_STATE_LIVE);           // the device starts on the live data page
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

// Path 4 (D-29): a page change cancels capture, the wait, and playback, and then moves.
static void path_cancel(void) {
    const coach_state_t busy[] = { COACH_STATE_CAPTURE, COACH_STATE_WAIT_ANSWER, COACH_STATE_PLAYBACK };
    for (size_t i = 0; i < sizeof busy / sizeof busy[0]; i++) {
        for (int up = 0; up < 2; up++) {
            coach_flow_t f; coach_flow_init(&f); f.state = busy[i];
            const owner_t before = busy[i] == COACH_STATE_CAPTURE ? OWNER_CAPTURE
                                 : busy[i] == COACH_STATE_PLAYBACK ? OWNER_PLAYBACK : OWNER_NONE;
            coach_step_t s = step(&f, up ? COACH_EVENT_UP_SHORT : COACH_EVENT_DOWN_SHORT);
            assert(f.state == (up ? COACH_STATE_TURNS : COACH_STATE_LIVE));   // D-30: Turns is before Coach
            assert(has_action(&s, COACH_ACTION_LINK_SEND_AGENT_CANCEL));
            assert(apply(before, &s) == OWNER_NONE);
        }
    }
    // Outside the Turns page, no hold other than the OK hold has a meaning.
    const coach_state_t all[] = { COACH_STATE_LIVE, COACH_STATE_REPORT, COACH_STATE_READY,
                                  COACH_STATE_CAPTURE, COACH_STATE_WAIT_ANSWER, COACH_STATE_PLAYBACK };
    for (size_t i = 0; i < sizeof all / sizeof all[0]; i++) {
        coach_flow_t f; coach_flow_init(&f); f.state = all[i];
        assert(!step(&f, COACH_EVENT_UP_HOLD).handled);
        assert(!step(&f, COACH_EVENT_DOWN_HOLD).handled);
    }
    // D-30: on the Turns page, OK and the two holds act inside the page, and start no audio.
    const coach_event_t inside[] = { COACH_EVENT_OK_SHORT, COACH_EVENT_UP_HOLD, COACH_EVENT_DOWN_HOLD };
    const coach_action_t act[] = { COACH_ACTION_TURN_NEXT, COACH_ACTION_TURN_PREV, COACH_ACTION_TURN_PAGE };
    for (size_t i = 0; i < 3; i++) {
        coach_flow_t f; coach_flow_init(&f); f.state = COACH_STATE_TURNS;
        coach_step_t s = step(&f, inside[i]);
        assert(s.handled && f.state == COACH_STATE_TURNS && has_action(&s, act[i]));
        assert(apply(OWNER_NONE, &s) == OWNER_NONE);
    }
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

// Path 6 (D-29, D-30): four flat pages in a loop. DOWN goes forward. UP goes back.
static void path_pages(void) {
    coach_flow_t f; coach_flow_init(&f);
    const coach_state_t order[] = { COACH_STATE_LIVE, COACH_STATE_REPORT, COACH_STATE_TURNS, COACH_STATE_READY,
                                    COACH_STATE_LIVE };
    const size_t n = sizeof order / sizeof order[0];
    for (size_t i = 1; i < n; i++) {
        step(&f, COACH_EVENT_DOWN_SHORT);
        assert(f.state == order[i]);
    }
    for (size_t i = n - 1; i > 0; i--) {
        step(&f, COACH_EVENT_UP_SHORT);
        assert(f.state == order[i - 1] || (i == 1 && f.state == COACH_STATE_READY));
    }
    f.state = COACH_STATE_LIVE;   step(&f, COACH_EVENT_UP_SHORT); assert(f.state == COACH_STATE_READY);
    f.state = COACH_STATE_READY;  step(&f, COACH_EVENT_UP_SHORT); assert(f.state == COACH_STATE_TURNS);
    f.state = COACH_STATE_TURNS;  step(&f, COACH_EVENT_UP_SHORT); assert(f.state == COACH_STATE_REPORT);
    f.state = COACH_STATE_REPORT; step(&f, COACH_EVENT_UP_SHORT); assert(f.state == COACH_STATE_LIVE);
    // A short OK is free for actions inside a page. Only the Turns page uses it (D-30).
    const coach_state_t pages[] = { COACH_STATE_LIVE, COACH_STATE_REPORT, COACH_STATE_READY };
    for (size_t i = 0; i < sizeof pages / sizeof pages[0]; i++) {
        f.state = pages[i];
        assert(!step(&f, COACH_EVENT_OK_SHORT).handled);
    }
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
// D-28: the phone opens the Report page from LIVE and READY only.
// Every other state ignores it, so it never stops a question or an answer.
static void path_show_report(void) {
    const coach_state_t go[] = { COACH_STATE_LIVE, COACH_STATE_READY };
    for (size_t i = 0; i < sizeof go / sizeof go[0]; i++) {
        coach_flow_t f; coach_flow_init(&f); f.state = go[i];
        coach_step_t s = step(&f, COACH_EVENT_SHOW_REPORT);
        assert(s.handled && f.state == COACH_STATE_REPORT);
        assert(apply(OWNER_NONE, &s) == OWNER_NONE);
    }
    const coach_state_t stay[] = { COACH_STATE_CAPTURE, COACH_STATE_WAIT_ANSWER, COACH_STATE_PLAYBACK };
    for (size_t i = 0; i < sizeof stay / sizeof stay[0]; i++) {
        coach_flow_t f; coach_flow_init(&f); f.state = stay[i];
        assert(!step(&f, COACH_EVENT_SHOW_REPORT).handled && f.state == stay[i]);
    }
    // On the Report page, a second request changes nothing.
    // An OK hold there opens the Coach page and starts no capture (D-29).
    coach_flow_t f; coach_flow_init(&f); f.state = COACH_STATE_REPORT;
    assert(!step(&f, COACH_EVENT_SHOW_REPORT).handled);
    coach_step_t s = step(&f, COACH_EVENT_OK_HOLD);
    assert(f.state == COACH_STATE_READY && !has_action(&s, COACH_ACTION_AUDIO_CAPTURE_START));
}

static void walk_invariants(void) {
    coach_flow_t f; coach_flow_init(&f);
    owner_t owner = OWNER_NONE;
    uint32_t seed = 12345u;
    for (int i = 0; i < 20000; i++) {
        seed = seed * 1103515245u + 12345u;
        coach_event_t e = (coach_event_t)((seed >> 16) % COACH_EVENT_COUNT);
        coach_step_t s = step(&f, e);
        owner = apply(owner, &s);
        if (f.state == COACH_STATE_LIVE || f.state == COACH_STATE_READY ||
            f.state == COACH_STATE_WAIT_ANSWER ||
            f.state == COACH_STATE_REPORT || f.state == COACH_STATE_TURNS) {
            assert(owner == OWNER_NONE);           // no audio outside capture and playback
        }
        if (f.state == COACH_STATE_CAPTURE) assert(owner == OWNER_CAPTURE);
        if (f.state == COACH_STATE_PLAYBACK) assert(owner == OWNER_PLAYBACK);
    }
}

int main(void) {
    path_ok_hold_opens_coach();
    path_capture_and_end();
    path_cancel();
    path_barge_in();
    path_pages();
    path_link_loss();
    path_show_report();
    walk_invariants();
    printf("coach path tests: PASS\n");
    return 0;
}
