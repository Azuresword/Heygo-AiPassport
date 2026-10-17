// tests/test_turn_view.c
// Host test for the Turns page view in main/ui/turn_view.c.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "turn_view.h"

static int s_fail;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); s_fail++; } } while (0)

#define ALL TURN_VIEW_DRAWN_ALL

// A run of n_turns turns. The cache holds group `group` in slot 0: 2 turns over 6 points,
// 0 to 250 downhill (y), and 0 to 255 to the sides (x).
static link_turns_t sample(uint8_t n_turns, uint8_t group) {
    link_turns_t t;
    memset(&t, 0, sizeof t);
    t.run_no = 4;
    t.source = BP_TURN_SRC_MEASURED;
    t.n_turns = n_turns;
    t.n_groups = (uint8_t)((n_turns + 7) / 8);
    link_turn_group_t *g = &t.slot[0];
    g->used = true;
    g->group = group;
    const uint8_t pts[6][2] = { { 128, 0 }, { 255, 50 }, { 128, 100 }, { 0, 150 }, { 128, 200 }, { 255, 250 } };
    g->n_points = 6;
    memcpy(g->points, pts, sizeof pts);
    g->n_turns = 2;
    g->turns[0] = (bp_turn_t){ 0, 2, BP_SIDE_TOE, 54, 55, 12 };
    g->turns[1] = (bp_turn_t){ 2, 4, BP_SIDE_HEEL, BP_U8_UNKNOWN, 51, BP_U8_UNKNOWN };
    return t;
}

static void test_no_data(void) {
    turn_view_t v;
    turn_view_build(&v, NULL, 0, ALL, true, 220, 120, 10);
    CHECK(v.n_path == 0 && v.n_turn == 0 && !v.has_head && !v.has_progress, "no shape draws nothing");
    CHECK(strcmp(v.title, "暂无弯型数据") == 0, "no shape shows the empty text");
    CHECK(strcmp(v.side, "SIDE --") == 0 && v.metrics[0] == '\0' && v.counter[0] == '\0', "no metrics");
    CHECK(strcmp(v.key_up, "▲报告") == 0 && v.key_ok[0] == '\0' && v.key_down[0] == '\0', "only UP has a hint");
    CHECK(turn_view_next(NULL, 3) == 0, "next with no data is 0");
}

static void test_fixed_map(void) {
    link_turns_t t = sample(2, 0);
    turn_view_t v;
    // Box 220 x 120, margin 10: packet 0 to 255 fills 200 x 100. The phone sets the scale.
    turn_view_build(&v, &t, 0, ALL, true, 220, 120, 10);
    CHECK(!v.drawing && !v.has_head, "all points are on the screen");
    CHECK(v.n_path == 6 << TURN_VIEW_SMOOTH, "each pass doubles the points");
    const int last = v.n_path - 1;
    CHECK(v.path[0].x == 10 && v.path[0].y == 60, "packet (128, 0) is the left middle");
    CHECK(v.path[last].x == 206 && v.path[last].y == 110, "packet (255, 250) is near the right bottom");
    int16_t min_y = 999, max_y = -999;
    for (int i = 0; i < v.n_path; i++) {
        if (v.path[i].y < min_y) min_y = v.path[i].y;
        if (v.path[i].y > max_y) max_y = v.path[i].y;
        if (i) CHECK(v.path[i].x >= v.path[i - 1].x, "downhill runs from left to right");
    }
    CHECK(min_y > 10 && max_y == 110, "smoothing cuts the corners and stays inside them");
    CHECK(!v.has_progress, "one group has no progress bar");
}

static void test_turn_and_texts(void) {
    link_turns_t t = sample(2, 0);
    turn_view_t v;
    turn_view_build(&v, &t, 0, ALL, true, 220, 120, 10);
    CHECK(strcmp(v.title, "第 4 趟 · 2 弯") == 0, "title");
    CHECK(strcmp(v.counter, "TURN 1/2") == 0 && strcmp(v.side, "TOE SIDE") == 0, "counter and side");
    CHECK(strcmp(v.metrics, "54 KM/H  55°  R 12 M") == 0, "metrics");
    CHECK(v.turn_first == 0 && v.n_turn == 3 << TURN_VIEW_SMOOTH, "turn 1 covers points 0 to 2");
    CHECK(v.source[0] == '\0' && v.msg[0] == '\0', "no tag and no message");
    CHECK(strcmp(v.key_up, "▲报告") == 0 && strcmp(v.key_ok, "●重放") == 0 && strcmp(v.key_down, "▼下一弯") == 0,
          "turn 1 of one page: UP goes back, OK replays, DOWN goes on");

    turn_view_build(&v, &t, 1, ALL, true, 220, 120, 10);
    CHECK(strcmp(v.side, "HEEL SIDE") == 0 && strcmp(v.counter, "TURN 2/2") == 0, "second turn");
    CHECK(strcmp(v.key_up, "▲上一弯") == 0 && v.key_down[0] == '\0', "the last turn: UP goes back one turn, DOWN has no action");
    CHECK(strcmp(v.metrics, "-- KM/H  51°  R -- M") == 0, "an unknown metric shows --, never 0");
    CHECK(v.turn_first == 2 << TURN_VIEW_SMOOTH && v.n_turn == 3 << TURN_VIEW_SMOOTH, "turn 2 covers points 2 to 4");

    turn_view_build(&v, &t, 9, ALL, true, 220, 120, 10);
    CHECK(strcmp(v.counter, "TURN 1/2") == 0, "a selection out of range selects turn 1");
    CHECK(turn_view_next(&t, 0) == 1 && turn_view_next(&t, 1) == 0, "next wraps to the first turn");
}

static void test_groups(void) {
    // 44 turns: 6 groups. The cache holds group 2: turns 17 to 24.
    link_turns_t t = sample(44, 2);
    t.slot[0].n_turns = 2;
    turn_view_t v;
    turn_view_build(&v, &t, 17, ALL, true, 220, 120, 10);
    CHECK(strcmp(v.counter, "TURN 18/44") == 0 && strcmp(v.side, "HEEL SIDE") == 0, "the counter counts in the run");
    CHECK(strcmp(v.key_up, "▲上一弯") == 0 && strcmp(v.key_ok, "●下一页") == 0 && strcmp(v.key_down, "▼下一弯") == 0,
          "a long run: UP and DOWN move one turn, OK one page");
    CHECK(v.has_progress && v.prog_x == 10 + 200 * 2 / 6 && v.prog_w == 200 / 6, "the bar marks group 3 of 6");

    // Group 0 is not cached.
    turn_view_build(&v, &t, 3, ALL, true, 220, 120, 10);
    CHECK(strcmp(v.msg, "加载中") == 0 && v.n_path == 0, "a missing group shows the load text");
    CHECK(strcmp(v.key_up, "▲上一弯") == 0 && strcmp(v.key_ok, "●下一页") == 0, "the keys still work while a page loads");
    turn_view_build(&v, &t, 0, ALL, true, 220, 120, 10);
    CHECK(strcmp(v.key_up, "▲报告") == 0, "turn 1: UP goes back to the Report page");
    turn_view_build(&v, &t, 3, ALL, false, 220, 120, 10);
    CHECK(strcmp(v.msg, "蓝牙断开") == 0, "a missing group with no link names the link");

    // The last group: DOWN has no action. The bar ends at the right end.
    turn_view_build(&v, &t, 43, ALL, true, 220, 120, 10);
    CHECK(v.key_down[0] == '\0' && v.prog_x + v.prog_w <= 210 && v.prog_x == 10 + 200 * 5 / 6, "the last turn of the last page");
    CHECK(turn_view_next(&t, 43) == 0 && turn_view_next(&t, 7) == 8, "next goes into the next group, and wraps");
}

static void test_drawing(void) {
    link_turns_t t = sample(2, 0);
    turn_view_t v;

    turn_view_build(&v, &t, 1, 0, true, 220, 120, 10);
    CHECK(v.drawing && !v.has_head && v.n_path == 0 && v.n_turn == 0, "no point draws no line and no rider");
    CHECK(v.counter[0] == '\0' && strcmp(v.key_ok, "●跳过") == 0, "while drawing, OK skips");

    turn_view_build(&v, &t, 1, 1, true, 220, 120, 10);
    CHECK(v.has_head && v.head.x == 10 && v.head.y == 60 && v.n_path == 0, "one point places the rider only");

    turn_view_build(&v, &t, 1, 2, true, 220, 120, 10);
    CHECK(v.n_path == 2 << TURN_VIEW_SMOOTH && v.head.x == v.path[v.n_path - 1].x && v.head.y == v.path[v.n_path - 1].y,
          "the rider sits at the end of the line");
    CHECK(v.head_frame == turn_view_frame(v.path[v.n_path - 1].x - 10, v.path[v.n_path - 1].y - 60), "the rider faces along the line");
    CHECK(strcmp(v.counter, "TURN 1/2") == 0 && v.turn_first == 0 && v.n_turn == v.n_path, "the mark ends at the head");

    turn_view_build(&v, &t, 0, 4, true, 220, 120, 10);
    CHECK(strcmp(v.counter, "TURN 2/2") == 0 && strcmp(v.side, "HEEL SIDE") == 0, "the texts follow the head, not sel");
    turn_view_build(&v, &t, 0, 3, true, 220, 120, 10);
    CHECK(strcmp(v.counter, "TURN 1/2") == 0, "point 2 ends turn 1");

    turn_view_build(&v, &t, 1, 6, true, 220, 120, 10);
    CHECK(!v.drawing && !v.has_head && strcmp(v.counter, "TURN 2/2") == 0 && strcmp(v.key_ok, "●重放") == 0,
          "after the last point, sel selects");
}

static void test_frames(void) {
    CHECK(turn_view_frame(0, 0) == 0, "no move gives frame 0");
    CHECK(turn_view_frame(10, 0) == 0, "right is frame 0");
    CHECK(turn_view_frame(0, 10) == 4, "down is frame 4");
    CHECK(turn_view_frame(-10, 0) == 8, "left is frame 8");
    CHECK(turn_view_frame(0, -10) == 12, "up is frame 12");
    CHECK(turn_view_frame(10, 10) == 2, "down right is frame 2");
    CHECK(turn_view_frame(10, -1) == 0 && turn_view_frame(10, -3) == 15, "a small turn up gives frame 0, then 15");
}

static void test_edge_cases(void) {
    link_turns_t t = sample(2, 0);
    turn_view_t v;
    t.source = BP_TURN_SRC_SCHEMATIC;
    t.run_no = BP_U8_UNKNOWN;
    turn_view_build(&v, &t, 0, ALL, true, 220, 120, 10);
    CHECK(strcmp(v.source, "示意") == 0, "a schematic shape shows the tag");
    CHECK(strcmp(v.title, "第 -- 趟 · 2 弯") == 0, "unknown run number");
    t.n_turns = 0;
    t.n_groups = 0;
    turn_view_build(&v, &t, 0, ALL, true, 220, 120, 10);
    CHECK(v.n_path == 0 && strcmp(v.title, "第 -- 趟 · 0 弯") == 0 && v.key_ok[0] == '\0', "a run with no turns");

    // The largest group: 8 turns and 33 points fill the buffer and stay in the box.
    memset(&t, 0, sizeof t);
    t.n_turns = 255;
    t.n_groups = 32;
    link_turn_group_t *g = &t.slot[0];
    g->used = true;
    g->group = 31;
    g->n_points = BP_GROUP_POINTS_MAX;
    for (int i = 0; i < g->n_points; i++) { g->points[i][0] = (uint8_t)(i % 2 ? 255 : 0); g->points[i][1] = (uint8_t)(i * 7); }
    g->n_turns = 7;   // 255 - 31 * 8
    for (int k = 0; k < 7; k++) g->turns[k] = (bp_turn_t){ (uint8_t)(4 * k), (uint8_t)(4 * k + 4), 0, 1, 2, 3 };
    g->turns[6].last = BP_GROUP_POINTS_MAX - 1;
    turn_view_build(&v, &t, 254, ALL, true, 220, 108, 10);
    CHECK(v.n_path == TURN_VIEW_PATH_MAX && strcmp(v.counter, "TURN 255/255") == 0, "the largest group fills the buffer");
    CHECK(v.turn_first + v.n_turn == v.n_path, "the last turn ends at the end of the path");
    for (int i = 0; i < v.n_path; i++)
        CHECK(v.path[i].x >= 10 && v.path[i].x <= 210 && v.path[i].y >= 10 && v.path[i].y <= 98, "inside the box");
}

int main(void) {
    test_no_data();
    test_fixed_map();
    test_turn_and_texts();
    test_groups();
    test_drawing();
    test_frames();
    test_edge_cases();
    if (s_fail) return EXIT_FAILURE;
    printf("turn view tests: PASS\n");
    return EXIT_SUCCESS;
}
