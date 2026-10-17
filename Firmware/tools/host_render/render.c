// tools/host_render/render.c
//
// Host program that draws the generated coach screens with the device LVGL
// configuration. render.py builds it and sends commands on stdin:
//
//   screen coach|ski|report|turns|exit   Delete the current screen and build this one.
//   set <subject> <value>   Call the generated setter. The value is the rest of the line.
//   label <widget> <text>   Set a label of the Ski or Report page, as the Coach task does.
//   panel metrics|report    Show the metric cells or the report lines of the Ski page.
//   shape <run> <source> <turns> <sel> <drawn> <permille> <link> <group> [<k> <turn>... <n> <x y>...]
//                           Put a TURN_SHAPE and one cached TURN_GROUP on the Turns page through
//                           turn_view.c and turn_page.c, as the Coach task does. turns -1: no shape.
//                           group -1: no group in the cache.
//   snap <name>             Draw the screen and write <outdir>/<name>.rgb565.
//   drop                    Delete the current screen and load a blank screen.
//   text body               Write the byte length and the bytes of the body label text.
//   mem <label>             Write the LVGL pool use.
//
// Each output line starts with a tag: MEM, BOX, SNAP, TEXT, LOCK, or ERR.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lvgl.h"
#include "coach_screens.h"
#include "coach_bindings.h"
#include "turn_page.h"
#include "turn_view.h"

#define HOR 240
#define VER 320

static uint16_t s_draw_buf[HOR * VER];
static uint16_t s_frame[HOR * VER];
static const char *s_outdir;

static enum { NONE, COACH, SKI, REPORT, TURNS, EXIT } s_kind = NONE;
static coach_screen_coach_t s_coach;
static coach_screen_ski_t s_ski;
static coach_screen_report_t s_report;
static coach_screen_turns_t s_turns;
static coach_screen_exit_t s_exit;
static lv_obj_t *s_blank;

static int s_takes, s_gives;

static bool count_lock(void) { s_takes++; return true; }
static void count_unlock(void) { s_gives++; }

static uint32_t tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    const uint16_t *src = (const uint16_t *)px;
    int32_t w = lv_area_get_width(area);
    for (int32_t y = area->y1; y <= area->y2; y++) {
        memcpy(&s_frame[y * HOR + area->x1], src, (size_t)w * 2);
        src += w;
    }
    lv_display_flush_ready(disp);
}

static void print_mem(const char *label)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    printf("MEM %s used=%u peak=%u total=%u biggest_free=%u frag=%u\n", label,
           (unsigned)(m.total_size - m.free_size), (unsigned)m.max_used,
           (unsigned)m.total_size, (unsigned)m.free_biggest_size, (unsigned)m.frag_pct);
}

static void print_box(const char *id, lv_obj_t *obj)
{
    // A hidden widget takes no room on the screen, so the layout check skips it.
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return;
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    printf("BOX %s x=%d y=%d w=%d h=%d\n", id, (int)a.x1, (int)a.y1,
           (int)lv_area_get_width(&a), (int)lv_area_get_height(&a));
}

static void drop_screen(void)
{
    // A screen cannot delete itself while it is active. Load a blank screen first.
    lv_screen_load(s_blank);
    switch (s_kind) {
    case COACH: coach_teardown(&s_coach); break;
    case SKI: coach_screen_ski_destroy(&s_ski); break;
    case REPORT: coach_screen_report_destroy(&s_report); break;
    case TURNS: coach_screen_turns_destroy(&s_turns); break;
    case EXIT: coach_screen_exit_destroy(&s_exit); break;
    case NONE: break;
    }
    s_kind = NONE;
}

static void load_screen(const char *name)
{
    drop_screen();
    if (strcmp(name, "coach") == 0) {
        coach_screen_coach_build(&s_coach);
        coach_bindings_attach(&s_coach);
        lv_screen_load(s_coach.screen);
        s_kind = COACH;
    } else if (strcmp(name, "ski") == 0) {
        coach_screen_ski_build(&s_ski);
        lv_screen_load(s_ski.screen);
        s_kind = SKI;
    } else if (strcmp(name, "report") == 0) {
        coach_screen_report_build(&s_report);
        lv_screen_load(s_report.screen);
        s_kind = REPORT;
    } else if (strcmp(name, "turns") == 0) {
        coach_screen_turns_build(&s_turns);
        lv_screen_load(s_turns.screen);
        s_kind = TURNS;
    } else if (strcmp(name, "exit") == 0) {
        coach_screen_exit_build(&s_exit);
        lv_screen_load(s_exit.screen);
        s_kind = EXIT;
    } else {
        printf("ERR unknown screen %s\n", name);
    }
}

static void set_subject(const char *id, const char *value)
{
    if (strcmp(id, "status_text") == 0) coach_set_status_text(value);
    else if (strcmp(id, "state_text") == 0) coach_set_state_text(value);
    else if (strcmp(id, "tag_text") == 0) coach_set_tag_text(value);
    else if (strcmp(id, "body_text") == 0) coach_set_body_text(value);
    else if (strcmp(id, "hint_text") == 0) coach_set_hint_text(value);
    else if (strcmp(id, "battery_text") == 0) coach_set_battery_text(value);
    else if (strcmp(id, "link_state") == 0) coach_set_link_state(atoi(value));
    else if (strcmp(id, "icon_state") == 0) coach_set_icon_state(atoi(value));
    else printf("ERR unknown subject %s\n", id);
}

// The data labels of the Ski page and the Report page. coach_app.c writes them.
static lv_obj_t *data_label(const char *id)
{
    if (s_kind == SKI) {
        if (strcmp(id, "dest") == 0) return s_ski.dest;
        if (strcmp(id, "sub") == 0) return s_ski.sub;
        if (strcmp(id, "big") == 0) return s_ski.big;
        if (strcmp(id, "unit") == 0) return s_ski.unit;
        if (strcmp(id, "drop") == 0) return s_ski.drop;
        if (strcmp(id, "dist") == 0) return s_ski.dist;
        if (strcmp(id, "turns") == 0) return s_ski.turns;
        if (strcmp(id, "time") == 0) return s_ski.time;
        if (strcmp(id, "problem") == 0) return s_ski.problem;
        if (strcmp(id, "tip") == 0) return s_ski.tip;
    } else if (s_kind == REPORT) {
        if (strcmp(id, "title") == 0) return s_report.title;
        if (strcmp(id, "iq") == 0) return s_report.iq;
        if (strcmp(id, "age") == 0) return s_report.age;
        if (strcmp(id, "problem") == 0) return s_report.problem;
        if (strcmp(id, "tip") == 0) return s_report.tip;
    }
    return NULL;
}

// Read the numbers of a shape command, check the group as link_core.c does, and draw the page
// through turn_view.c and turn_page.c, as the Coach task does.
static void put_shape(char *args)
{
    long v[9 + 6 * BP_GROUP_TURNS + 1 + 2 * BP_GROUP_POINTS_MAX];
    size_t n = 0;
    for (char *tok = strtok(args, " "); tok && n < sizeof v / sizeof v[0]; tok = strtok(NULL, " ")) v[n++] = strtol(tok, NULL, 10);
    static link_turns_t t;
    memset(&t, 0, sizeof t);
    size_t i = 0;
    if (n < 8) { printf("ERR shape needs run, source, turns, sel, drawn, permille, link, and group\n"); return; }
    t.run_no = (uint8_t)v[i++];
    t.source = (uint8_t)v[i++];
    const long n_turns = v[i++];
    const uint8_t sel = (uint8_t)v[i++];
    const uint8_t drawn = (uint8_t)v[i++];
    const int32_t permille = (int32_t)v[i++];
    const bool link_up = v[i++] != 0;
    const long group = v[i++];
    t.n_turns = (uint8_t)(n_turns < 0 ? 0 : n_turns);
    t.n_groups = (uint8_t)((t.n_turns + BP_GROUP_TURNS - 1) / BP_GROUP_TURNS);
    if (group >= 0) {
        link_turn_group_t *g = &t.slot[0];
        if (n < i + 1) { printf("ERR shape group turns\n"); return; }
        g->used = true;
        g->group = (uint8_t)group;
        g->n_turns = (uint8_t)v[i++];
        if (g->n_turns > BP_GROUP_TURNS || n < i + 6u * g->n_turns + 1) { printf("ERR shape group turns\n"); return; }
        for (uint8_t k = 0; k < g->n_turns; k++, i += 6) {
            g->turns[k] = (bp_turn_t){ (uint8_t)v[i], (uint8_t)v[i + 1], (uint8_t)v[i + 2],
                                       (uint8_t)v[i + 3], (uint8_t)v[i + 4], (uint8_t)v[i + 5] };
        }
        g->n_points = (uint8_t)v[i++];
        if (g->n_points > BP_GROUP_POINTS_MAX || n != i + 2u * g->n_points) { printf("ERR shape group points\n"); return; }
        for (uint8_t k = 0; k < g->n_points; k++) { g->points[k][0] = (uint8_t)v[i++]; g->points[k][1] = (uint8_t)v[i++]; }
        for (uint8_t k = 0; k < g->n_turns; k++) {
            if (g->turns[k].first >= g->turns[k].last || g->turns[k].last >= g->n_points) { printf("ERR shape turn %u\n", k); return; }
        }
    }
    static turn_view_t view;
    turn_view_build(&view, n_turns >= 0 ? &t : NULL, sel, drawn, link_up, TURN_PAGE_BOX_W, TURN_PAGE_BOX_H, TURN_PAGE_MARGIN);
    turn_page_apply(&s_turns, &view);
    if (!view.drawing) turn_page_place_rider(&s_turns, permille);
}

static void snap(const char *name)
{
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(NULL);

    char path[512];
    snprintf(path, sizeof path, "%s/%s.rgb565", s_outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { printf("ERR cannot write %s\n", path); return; }
    fwrite(s_frame, sizeof s_frame, 1, f);
    fclose(f);
    printf("SNAP %s\n", name);

    switch (s_kind) {
    case COACH:
        print_box("mode", s_coach.mode); print_box("status", s_coach.status);
        print_box("battery", s_coach.battery); print_box("dest", s_coach.dest);
        print_box("icon", s_coach.icon);
        print_box("tag", s_coach.tag);
        print_box("state", s_coach.state); print_box("body", s_coach.body);
        print_box("hint", s_coach.hint);
        break;
    case SKI:
        print_box("mode", s_ski.mode); print_box("battery", s_ski.battery);
        print_box("dest", s_ski.dest);
        print_box("sub", s_ski.sub); print_box("big", s_ski.big);
        print_box("unit", s_ski.unit);
        print_box("k_drop", s_ski.k_drop); print_box("drop", s_ski.drop);
        print_box("k_dist", s_ski.k_dist); print_box("dist", s_ski.dist);
        print_box("k_turns", s_ski.k_turns); print_box("turns", s_ski.turns);
        print_box("k_time", s_ski.k_time); print_box("time", s_ski.time);
        print_box("problem", s_ski.problem); print_box("tip", s_ski.tip);
        print_box("hint", s_ski.hint);
        break;
    case REPORT:
        print_box("mode", s_report.mode); print_box("battery", s_report.battery);
        print_box("dest", s_report.dest); print_box("title", s_report.title);
        print_box("iq", s_report.iq); print_box("iq_name", s_report.iq_name);
        print_box("age", s_report.age);
        print_box("problem", s_report.problem); print_box("tip", s_report.tip);
        print_box("hint", s_report.hint);
        break;
    case TURNS:
        // The lines, the rider, the bar, and msg sit inside shape_panel, so the layout check skips them.
        print_box("mode", s_turns.mode); print_box("battery", s_turns.battery);
        print_box("dest", s_turns.dest); print_box("title", s_turns.title);
        print_box("source", s_turns.source); print_box("shape_panel", s_turns.shape_panel);
        print_box("side", s_turns.side); print_box("counter", s_turns.counter);
        print_box("metrics", s_turns.metrics);   // msg sits inside shape_panel, as the lines do.
        print_box("key_up", s_turns.key_up); print_box("key_ok", s_turns.key_ok); print_box("key_down", s_turns.key_down);
        break;
    case EXIT:
        print_box("mode", s_exit.mode); print_box("battery", s_exit.battery);
        print_box("dest", s_exit.dest); print_box("icon", s_exit.icon);
        print_box("title", s_exit.title); print_box("primary", s_exit.primary);
        print_box("secondary", s_exit.secondary); print_box("hint", s_exit.hint);
        break;
    case NONE: break;
    }
    print_mem(name);
}

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: render <outdir> < commands\n"); return 2; }
    s_outdir = argv[1];

    lv_init();
    lv_tick_set_cb(tick_ms);
    print_mem("after_init");

    lv_display_t *disp = lv_display_create(HOR, VER);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, s_draw_buf, NULL, sizeof s_draw_buf, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, flush_cb);
    s_blank = lv_obj_create(NULL);
    coach_bindings_set_lock(count_lock, count_unlock);
    print_mem("after_display");

    char line[1024];
    while (fgets(line, sizeof line, stdin)) {
        line[strcspn(line, "\n")] = 0;
        char *cmd = strtok(line, " ");
        if (!cmd) continue;
        char *arg = strtok(NULL, " ");
        char *rest = strtok(NULL, "");
        if (strcmp(cmd, "screen") == 0 && arg) load_screen(arg);
        else if (strcmp(cmd, "set") == 0 && arg) set_subject(arg, rest ? rest : "");
        else if (strcmp(cmd, "label") == 0 && arg) {
            lv_obj_t *w = data_label(arg);
            if (w) lv_label_set_text(w, rest ? rest : "");
            else printf("ERR unknown label %s\n", arg);
        }
        else if (strcmp(cmd, "panel") == 0 && arg && s_kind == SKI) {
            const bool report = strcmp(arg, "report") == 0;
            lv_obj_t *cells[] = { s_ski.k_drop, s_ski.drop, s_ski.k_dist, s_ski.dist,
                                  s_ski.k_turns, s_ski.turns, s_ski.k_time, s_ski.time };
            for (size_t i = 0; i < sizeof cells / sizeof cells[0]; i++) {
                if (report) lv_obj_add_flag(cells[i], LV_OBJ_FLAG_HIDDEN);
                else lv_obj_remove_flag(cells[i], LV_OBJ_FLAG_HIDDEN);
            }
            if (report) { lv_obj_remove_flag(s_ski.problem, LV_OBJ_FLAG_HIDDEN); lv_obj_remove_flag(s_ski.tip, LV_OBJ_FLAG_HIDDEN); }
            else { lv_obj_add_flag(s_ski.problem, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(s_ski.tip, LV_OBJ_FLAG_HIDDEN); }
        }
        else if (strcmp(cmd, "shape") == 0 && s_kind == TURNS) {
            // strtok already cut the line after the first number. Glue it back.
            char buf[1024];
            snprintf(buf, sizeof buf, "%s %s", arg ? arg : "", rest ? rest : "");
            put_shape(buf);
        }
        else if (strcmp(cmd, "snap") == 0 && arg) snap(arg);
        else if (strcmp(cmd, "mem") == 0 && arg) print_mem(arg);
        else if (strcmp(cmd, "drop") == 0) drop_screen();
        else if (strcmp(cmd, "text") == 0 && s_kind == COACH) {
            const char *t = lv_label_get_text(s_coach.body);
            printf("TEXT %zu ", strlen(t));
            for (const char *c = t; *c; c++) printf("%02x", (unsigned char)*c);
            printf("\n");
        }
        else printf("ERR bad command %s\n", cmd);
    }

    drop_screen();
    print_mem("after_teardown");
    printf("LOCK takes=%d gives=%d\n", s_takes, s_gives);
    return 0;
}
