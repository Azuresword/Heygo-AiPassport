// tests/test_turn_view.c
// Host test for the Turns page view in main/ui/turn_view.c.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "turn_view.h"

static int s_fail;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); s_fail++; } } while (0)

static link_turns_t sample(void) {
    // A run that goes 200 downhill (y 0..200) and swings 100 to the sides (x 40..140).
    link_turns_t t;
    memset(&t, 0, sizeof t);
    t.run_no = 4;
    t.source = BP_TURN_SRC_MEASURED;
    const uint8_t pts[6][2] = { { 90, 0 }, { 140, 40 }, { 90, 80 }, { 40, 120 }, { 90, 160 }, { 140, 200 } };
    t.n_points = 6;
    memcpy(t.points, pts, sizeof pts);
    t.n_turns = 2;
    t.turns[0] = (bp_turn_t){ 0, 2, BP_SIDE_TOE, 54, 55, 12 };
    t.turns[1] = (bp_turn_t){ 2, 4, BP_SIDE_HEEL, BP_U8_UNKNOWN, 51, BP_U8_UNKNOWN };
    return t;
}

static void test_no_data(void) {
    turn_view_t v;
    turn_view_build(&v, NULL, 0, 220, 120, 10);
    CHECK(v.n_path == 0 && v.n_turn == 0, "no shape draws nothing");
    CHECK(strcmp(v.title, "暂无弯型数据") == 0, "no shape shows the empty text");
    CHECK(strcmp(v.side, "SIDE --") == 0 && v.metrics[0] == '\0' && v.counter[0] == '\0', "no metrics");
    CHECK(turn_view_next(NULL, 3) == 0, "next with no data is 0");
}

static void test_fit_and_aspect(void) {
    link_turns_t t = sample();
    turn_view_t v;
    // The page turns the run: downhill goes right. 200 x 100 on screen.
    // Box 220 x 140, margin 10: 200 x 120 for the shape. Width limits the scale: 200 / 200.
    turn_view_build(&v, &t, 0, 220, 140, 10);
    CHECK(v.n_path == 6 << TURN_VIEW_SMOOTH, "each pass doubles the points");
    int16_t min_x = 999, max_x = -999, min_y = 999, max_y = -999;
    for (int i = 0; i < v.n_path; i++) {
        if (v.path[i].x < min_x) min_x = v.path[i].x;
        if (v.path[i].x > max_x) max_x = v.path[i].x;
        if (v.path[i].y < min_y) min_y = v.path[i].y;
        if (v.path[i].y > max_y) max_y = v.path[i].y;
    }
    const int last = v.n_path - 1;
    CHECK(min_x == 10 && max_x == 210, "the downhill axis fills the box width inside the margin");
    CHECK(v.path[0].x == 10 && v.path[0].y == 70 && v.path[last].x == 210 && v.path[last].y == 120,
          "the ends stay: the scale is 1, and the side axis starts at 20 (centered)");
    CHECK(min_y > 20 && max_y == 120 && max_y - min_y < 100, "smoothing cuts the corners and stays inside them");
    for (int i = 1; i < v.n_path; i++) CHECK(v.path[i].x >= v.path[i - 1].x, "downhill runs from left to right");
    CHECK(v.path[1].y > v.path[0].y, "a larger side value draws lower");
}

static void test_turn_and_texts(void) {
    link_turns_t t = sample();
    turn_view_t v;
    turn_view_build(&v, &t, 0, 220, 120, 10);
    CHECK(strcmp(v.title, "第 4 趟 · 2 弯") == 0, "title");
    CHECK(strcmp(v.counter, "TURN 1/2") == 0, "counter");
    CHECK(strcmp(v.side, "TOE SIDE") == 0, "toe side");
    CHECK(strcmp(v.metrics, "54 KM/H  55°  R 12 M") == 0, "metrics");
    CHECK(v.turn_first == 0 && v.n_turn == 3 << TURN_VIEW_SMOOTH, "turn 1 covers packet points 0 to 2");
    CHECK(v.source[0] == '\0', "a measured shape has no tag");

    turn_view_build(&v, &t, 1, 220, 120, 10);
    CHECK(strcmp(v.side, "HEEL SIDE") == 0 && strcmp(v.counter, "TURN 2/2") == 0, "second turn");
    CHECK(strcmp(v.metrics, "-- KM/H  51°  R -- M") == 0, "an unknown metric shows --, never 0");
    CHECK(v.turn_first == 2 << TURN_VIEW_SMOOTH && v.n_turn == 3 << TURN_VIEW_SMOOTH, "turn 2 covers packet points 2 to 4");
    CHECK(v.turn_first + v.n_turn <= v.n_path, "the turn stays inside the path");

    turn_view_build(&v, &t, 9, 220, 120, 10);
    CHECK(strcmp(v.counter, "TURN 1/2") == 0, "a selection out of range selects turn 1");
    CHECK(turn_view_next(&t, 0) == 1 && turn_view_next(&t, 1) == 0, "next wraps to the first turn");
}

static void test_edge_cases(void) {
    link_turns_t t = sample();
    turn_view_t v;
    t.source = BP_TURN_SRC_SCHEMATIC;
    t.run_no = BP_U8_UNKNOWN;
    t.n_turns = 0;
    turn_view_build(&v, &t, 0, 220, 120, 10);
    CHECK(strcmp(v.source, "示意") == 0, "a schematic shape shows the tag");
    CHECK(strcmp(v.title, "第 -- 趟 · 0 弯") == 0, "unknown run number");
    CHECK(v.n_path == 6 << TURN_VIEW_SMOOTH && v.n_turn == 0 && strcmp(v.side, "SIDE --") == 0, "path without turns");

    // A straight run and a single repeated point must not divide by zero.
    t = sample();
    for (int i = 0; i < t.n_points; i++) t.points[i][0] = 77;
    turn_view_build(&v, &t, 0, 220, 120, 10);
    for (int i = 0; i < v.n_path; i++) CHECK(v.path[i].y == 60, "a straight run sits in the middle");
    for (int i = 0; i < t.n_points; i++) { t.points[i][0] = 5; t.points[i][1] = 5; }
    turn_view_build(&v, &t, 0, 220, 120, 10);
    CHECK(v.path[0].x == 110 && v.path[0].y == 60, "a single point sits in the center");
}

static void test_largest(void) {
    // 40 points and 12 turns: the buffer holds the smoothed path, and the last turn ends at its end.
    link_turns_t t;
    memset(&t, 0, sizeof t);
    t.n_points = BP_TURN_POINTS_MAX;
    for (int i = 0; i < t.n_points; i++) { t.points[i][0] = (uint8_t)(i % 2 ? 200 : 50); t.points[i][1] = (uint8_t)(i * 6); }
    t.n_turns = BP_TURNS_MAX;
    for (int k = 0; k < t.n_turns; k++) t.turns[k] = (bp_turn_t){ (uint8_t)(3 * k), (uint8_t)(3 * k + 3), 0, 1, 2, 3 };
    t.turns[11].last = BP_TURN_POINTS_MAX - 1;
    turn_view_t v;
    turn_view_build(&v, &t, 11, 220, 108, 10);
    CHECK(v.n_path == TURN_VIEW_PATH_MAX, "the largest shape fills the buffer");
    CHECK(v.turn_first + v.n_turn == v.n_path, "the last turn ends at the end of the path");
    for (int i = 0; i < v.n_path; i++)
        CHECK(v.path[i].x >= 10 && v.path[i].x <= 210 && v.path[i].y >= 10 && v.path[i].y <= 98, "inside the box");
}

int main(void) {
    test_no_data();
    test_fit_and_aspect();
    test_turn_and_texts();
    test_edge_cases();
    test_largest();
    if (s_fail) return EXIT_FAILURE;
    printf("turn view tests: PASS\n");
    return EXIT_SUCCESS;
}
