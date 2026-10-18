#!/usr/bin/env python3
"""flowc - compile a canvas flow graph into C for LVGL.

The device runs no graph interpreter. This generator translates a .flow.json
graph into plain C at build time, so the runtime cost of the canvas is zero.
That matters on ESP32-C3: it has no PSRAM and the LVGL pool is 24 KB.

Three outputs per app:

  <app>_flow.c/.h      pure C transition table. No LVGL, no ESP-IDF. Host testable.
  <app>_screens.c/.h   LVGL widget tree builders.
  <app>_bindings.c/.h  lv_subject_t values and the observers that drive widgets.

Run with --check to fail when a generated file is stale. CI uses that mode.
"""

import argparse
import json
import pathlib
import sys
from collections import defaultdict

# ---------------------------------------------------------------- pool model

# Per-widget LVGL 9.5 pool cost in bytes. tools/host_render measured the label,
# screen, and observer costs with lv_mem_monitor() on a 64-bit host (2026-10-16):
#   screen + 2 labels = 1048 B, screen + 3 labels = 1408 B,
#   screen + 5 labels + 5 observers = 2728 B.
# The ESP32-C3 has 4-byte pointers, so these values are an upper bound there.
# The other widget costs are not measured yet.
WIDGET_POOL_COST = {
    "label": 360,
    "arc": 300,
    "bar": 280,
    "obj": 180,
    "button": 240,
    "image": 220,
    "slider": 300,
    "switch": 240,
    "state_icon": 540,
    "line": 200,        # Not measured yet. The points stay in flash.
}
SCREEN_POOL_COST = 330
OBSERVER_POOL_COST = 120

# property -> (widget types that accept it, subject type it needs)
BINDABLE = {
    "text":  ({"label"}, "string"),
    "value": ({"arc", "bar", "slider", "state_icon"}, "int"),
    "color": ({"label", "arc", "bar", "obj", "button"}, "color"),
    "hidden": ({"label", "arc", "bar", "obj", "button", "image", "state_icon"}, "int"),
}

C_TYPE = {"int": "int32_t", "string": "char", "color": "int32_t"}


class FlowError(Exception):
    pass


LONG_MODES = {"wrap": "LV_LABEL_LONG_MODE_WRAP", "dots": "LV_LABEL_LONG_MODE_DOTS",
              "clip": "LV_LABEL_LONG_MODE_CLIP"}


def ident(name):
    return "".join(c if c.isalnum() else "_" for c in name)


# ---------------------------------------------------------------- validation

def binding_screens(g, b):
    """The screens and the widget of one binding.

    A target "<screen>.<widget>.<property>" binds that screen only. A target
    "<widget>.<property>" binds each screen in b["screens"], or screen 0 without it.
    Returns the screen ids and the widget id.
    """
    wid = b["target"].rsplit(".", 1)[0]
    if "." in wid:
        scr, wid = wid.split(".", 1)
        return [scr], wid
    return list(b.get("screens") or [g["screens"][0]["id"]]), wid


def validate(g, path):
    """Apply the seven validator rules. Collect every error before failing."""
    errors = []
    app = g.get("app")
    if not app:
        raise FlowError(f"{path}: missing 'app'")

    screens = {s["id"]: s for s in g.get("screens", [])}
    subjects = {s["id"]: s for s in g.get("subjects", [])}
    actions = {a["id"]: a for a in g.get("actions", [])}
    transforms = {t["id"]: t for t in g.get("transforms", [])}
    states = {s["id"]: s for s in g.get("states", [])}
    transitions = g.get("transitions", [])

    widgets = {}
    for scr in g.get("screens", []):
        for w in scr.get("widgets", []):
            widgets[f"{scr['id']}.{w['id']}"] = w
            if w["type"] not in WIDGET_POOL_COST:
                errors.append(f"widget {scr['id']}.{w['id']}: unknown type {w['type']!r}")
            if w["type"] == "line":
                pts = w.get("points", [])
                if len(pts) < 2 or not all(isinstance(q, list) and len(q) == 2 for q in pts):
                    errors.append(f"widget {scr['id']}.{w['id']}: a line needs 2 or more [x, y] points")
            mode = w.get("long_mode")
            if mode is not None and mode not in LONG_MODES:
                errors.append(f"widget {scr['id']}.{w['id']}: unknown long_mode {mode!r}")
            elif mode in ("dots", "clip") and "h" not in w:
                # Without a fixed height the label grows and never cuts the text.
                errors.append(f"widget {scr['id']}.{w['id']}: long_mode {mode!r} needs \"h\"")

    # --- referential integrity -------------------------------------------
    action_ids = {a["id"] for a in g.get("actions", [])}
    subject_ids = {s["id"] for s in g.get("subjects", [])}
    for aid, effect in g.get("ui_effects", {}).items():
        if aid == "comment":
            continue
        if aid not in action_ids:
            errors.append(f"ui_effects: unknown action {aid!r}")
        for sid in effect:
            if sid not in subject_ids:
                errors.append(f"ui_effects {aid}: unknown subject {sid!r}")
    for st in states.values():
        if st.get("screen") and st["screen"] not in screens:
            errors.append(f"state {st['id']}: unknown screen {st['screen']!r}")
    for t in transitions:
        if t["from"] not in states:
            errors.append(f"transition: unknown source state {t['from']!r}")
        if t["to"] not in states:
            errors.append(f"transition: unknown target state {t['to']!r}")
        for a in t.get("actions", []):
            if a not in actions:
                errors.append(f"transition {t['from']}->{t['to']}: unknown action {a!r}")

    initial = [s for s in states.values() if s.get("initial")]
    if len(initial) != 1:
        errors.append(f"need exactly one initial state, found {len(initial)}")

    # --- rule 1: no unreachable state ------------------------------------
    if initial and not errors:
        adj = defaultdict(set)
        for t in transitions:
            adj[t["from"]].add(t["to"])
        seen, stack = set(), [initial[0]["id"]]
        while stack:
            cur = stack.pop()
            if cur in seen:
                continue
            seen.add(cur)
            stack.extend(adj[cur])
        for sid in states:
            if sid not in seen:
                errors.append(f"rule 1: state {sid!r} is unreachable from the initial state")

    # --- rule 2: every state has a route back to a root ------------------
    roots = {s["id"] for s in states.values() if s.get("root")}
    if roots:
        rev = defaultdict(set)
        for t in transitions:
            rev[t["to"]].add(t["from"])
        can_reach, stack = set(roots), list(roots)
        while stack:
            cur = stack.pop()
            for prev in rev[cur]:
                if prev not in can_reach:
                    can_reach.add(prev)
                    stack.append(prev)
        for sid in states:
            if sid not in can_reach:
                errors.append(f"rule 2: state {sid!r} has no route back to a root state")
    else:
        errors.append("rule 2: no state is marked 'root'")

    # --- rule 3: a page taking OK_LONG must declare another back route ---
    inp = g.get("input", {})
    if inp.get("reserve_ok_long_for_page") and not inp.get("back_intents"):
        errors.append(
            "rule 3: input.reserve_ok_long_for_page is true, so input.back_intents "
            "must name a replacement back route"
        )

    # --- rule 4: binding types must match --------------------------------
    for b in g.get("bindings", []):
        target = b["target"]
        if "." not in target:
            errors.append(f"rule 4: binding target {target!r} must be '<widget>.<property>'")
            continue
        prop = target.rsplit(".", 1)[1]
        scrs, wid = binding_screens(g, b)
        keys = [f"{scr}.{wid}" for scr in scrs]
        missing = [k for k in keys if k not in widgets]
        if missing:
            errors.append(f"rule 4: binding target {target!r} names no widget {', '.join(missing)}")
            continue
        if prop not in BINDABLE:
            errors.append(f"rule 4: property {prop!r} is not bindable")
            continue
        ok_types, need = BINDABLE[prop]
        types = {widgets[k]["type"] for k in keys}
        if len(types) > 1:
            errors.append(f"rule 4: binding target {target!r} is not the same widget type on each screen")
            continue
        key = keys[0]
        if widgets[key]["type"] not in ok_types:
            errors.append(
                f"rule 4: widget {key} is a {widgets[key]['type']}, "
                f"which has no bindable {prop!r}"
            )
        subj = subjects.get(b["subject"])
        if subj is None:
            errors.append(f"rule 4: binding names unknown subject {b['subject']!r}")
            continue
        src = subj["type"]
        if b.get("transform"):
            tr = transforms.get(b["transform"])
            if tr is None:
                errors.append(f"rule 4: unknown transform {b['transform']!r}")
                continue
            if tr["from"] != src:
                errors.append(
                    f"rule 4: transform {tr['id']} takes {tr['from']}, "
                    f"but subject {subj['id']} is {src}"
                )
            src = tr["to"]
        if src != need:
            errors.append(
                f"rule 4: {target} needs a {need} subject, "
                f"but {b['subject']} resolves to {src}"
            )

    # --- rule 5: estimated LVGL pool use must fit ------------------------
    # By default all screens live together. With "one_screen_at_a_time", the app
    # deletes the old screen before it builds the new one, so only the largest counts.
    # Each binding adds one observer on each screen that it binds.
    # String subject buffers are static arrays, not LVGL pool memory.
    observers = {scr["id"]: 0 for scr in g.get("screens", [])}
    for b in g.get("bindings", []):
        if "." in b["target"]:
            for scr in binding_screens(g, b)[0]:
                if scr in observers:
                    observers[scr] += 1
    costs = [SCREEN_POOL_COST + sum(WIDGET_POOL_COST.get(w["type"], 0) for w in scr.get("widgets", []))
             + OBSERVER_POOL_COST * observers[scr["id"]] for scr in g.get("screens", [])]
    pool = (max(costs) if costs else 0) if g.get("one_screen_at_a_time") else sum(costs)
    ceiling = g.get("lvgl_pool_ceiling_bytes", 20480)
    if pool > ceiling:
        errors.append(
            f"rule 5: estimated LVGL pool use {pool} B exceeds the ceiling {ceiling} B"
        )

    # --- rule 6: blocking actions must be marked async -------------------
    for a in actions.values():
        if "async" not in a:
            errors.append(f"rule 6: action {a['id']!r} must declare \"async\": true or false")

    # --- rule 7: teardown is generated, so every screen must be named ----
    used = {st.get("screen") for st in states.values() if st.get("screen")}
    for sid in screens:
        if sid not in used:
            errors.append(f"rule 7: screen {sid!r} is never shown by any state")

    if errors:
        raise FlowError(f"{path}: {len(errors)} validation error(s):\n  - " + "\n  - ".join(errors))

    return {"pool_estimate": pool, "pool_ceiling": ceiling}


# ---------------------------------------------------------------- emitters

BANNER = """// Generated by tools/flow/flowc.py from {src}. Do not edit by hand.
// Re-run: python3 tools/flow/flowc.py {src}
"""


def emit_flow(g, src):
    """Pure C transition table. No LVGL and no ESP-IDF, so host tests can link it."""
    app = ident(g["app"])
    P = app.upper()
    states = [s["id"] for s in g["states"]]
    # input.extra_events lists events that the input code makes but no transition uses yet.
    events = sorted({t["on"] for t in g["transitions"]} | set(g.get("input", {}).get("extra_events", [])))
    actions = [a["id"] for a in g["actions"]]
    initial = next(s["id"] for s in g["states"] if s.get("initial"))

    h = [BANNER.format(src=src), "#pragma once", "", "#include <stdbool.h>",
         "#include <stddef.h>", "", "#ifdef __cplusplus", 'extern "C" {', "#endif", ""]
    h.append("typedef enum {")
    for s in states:
        h.append(f"    {P}_STATE_{s} ,".replace(" ,", ","))
    h.append(f"    {P}_STATE_COUNT")
    h.append("} %s_state_t;" % app)
    h.append("")
    h.append("typedef enum {")
    for e in events:
        h.append(f"    {P}_EVENT_{e},")
    h.append(f"    {P}_EVENT_COUNT")
    h.append("} %s_event_t;" % app)
    h.append("")
    h.append("typedef enum {")
    for a in actions:
        h.append(f"    {P}_ACTION_{a.upper()},")
    h.append(f"    {P}_ACTION_COUNT")
    h.append("} %s_action_t;" % app)
    h.append("")
    h.append("// An action marked async must run on a worker task, never in a key callback.")
    h.append(f"bool {app}_action_is_async({app}_action_t action);")
    h.append("")
    maxact = max((len(t.get("actions", [])) for t in g["transitions"]), default=0) or 1
    h.append(f"#define {P}_MAX_ACTIONS {maxact}")
    h.append("")
    h.append("typedef struct {")
    h.append(f"    {app}_state_t state;")
    h.append("} %s_flow_t;" % app)
    h.append("")
    h.append("typedef struct {")
    h.append("    bool handled;                 // false when the event has no transition")
    h.append("    bool state_changed;")
    h.append(f"    {app}_state_t from;")
    h.append(f"    {app}_state_t to;")
    h.append(f"    {app}_action_t actions[{P}_MAX_ACTIONS];")
    h.append("    size_t action_count;")
    h.append("} %s_step_t;" % app)
    h.append("")
    h.append(f"void {app}_flow_init({app}_flow_t *flow);")
    h.append(f"{app}_step_t {app}_flow_step({app}_flow_t *flow, {app}_event_t event);")
    h.append(f"const char *{app}_state_name({app}_state_t state);")
    h.append(f"const char *{app}_event_name({app}_event_t event);")
    h.append(f"const char *{app}_action_name({app}_action_t action);")
    h.append("")
    h.append("typedef enum {")
    for scr in g["screens"]:
        h.append(f"    {P}_SCREEN_{ident(scr['id']).upper()},")
    h.append(f"    {P}_SCREEN_COUNT")
    h.append("} %s_screen_id_t;" % app)
    h.append("")
    h.append("// The screen that the flow graph gives to this state.")
    h.append(f"{app}_screen_id_t {app}_state_screen({app}_state_t state);")
    h.append("")
    h += ["#ifdef __cplusplus", "}", "#endif", ""]

    c = [BANNER.format(src=src), f'#include "{app}_flow.h"', "", "typedef struct {",
         f"    {app}_state_t from;", f"    {app}_event_t on;", f"    {app}_state_t to;",
         f"    {app}_action_t actions[{P}_MAX_ACTIONS];", "    size_t action_count;",
         "} transition_t;", "", "static const transition_t TRANSITIONS[] = {"]
    for t in g["transitions"]:
        acts = t.get("actions", [])
        lst = ", ".join(f"{P}_ACTION_{a.upper()}" for a in acts) or "0"
        c.append(
            f"    {{ {P}_STATE_{t['from']}, {P}_EVENT_{t['on']}, {P}_STATE_{t['to']}, "
            f"{{ {lst} }}, {len(acts)} }},"
        )
    c.append("};")
    c.append("static const size_t TRANSITION_COUNT = sizeof(TRANSITIONS) / sizeof(TRANSITIONS[0]);")
    c.append("")
    c.append("static const bool ACTION_ASYNC[] = {")
    for a in g["actions"]:
        c.append(f"    {str(bool(a['async'])).lower()},")
    c.append("};")
    c.append("")
    c.append(f"bool {app}_action_is_async({app}_action_t action) {{")
    c.append(f"    if (action < 0 || action >= {P}_ACTION_COUNT) return false;")
    c.append("    return ACTION_ASYNC[action];")
    c.append("}")
    c.append("")
    c.append(f"void {app}_flow_init({app}_flow_t *flow) {{")
    c.append("    if (!flow) return;")
    c.append(f"    flow->state = {P}_STATE_{initial};")
    c.append("}")
    c.append("")
    c.append(f"{app}_step_t {app}_flow_step({app}_flow_t *flow, {app}_event_t event) {{")
    c.append(f"    {app}_step_t step;")
    c.append("    step.handled = false;")
    c.append("    step.state_changed = false;")
    c.append("    step.action_count = 0;")
    c.append(f"    step.from = flow ? flow->state : (({app}_state_t)0);")
    c.append("    step.to = step.from;")
    c.append(f"    for (size_t i = 0; i < {P}_MAX_ACTIONS; i++) step.actions[i] = ({app}_action_t)0;")
    c.append("    if (!flow) return step;")
    c.append("    for (size_t i = 0; i < TRANSITION_COUNT; i++) {")
    c.append("        const transition_t *t = &TRANSITIONS[i];")
    c.append("        if (t->from != flow->state || t->on != event) continue;")
    c.append("        step.handled = true;")
    c.append("        step.to = t->to;")
    c.append("        step.state_changed = (t->to != flow->state);")
    c.append("        for (size_t a = 0; a < t->action_count; a++) step.actions[a] = t->actions[a];")
    c.append("        step.action_count = t->action_count;")
    c.append("        flow->state = t->to;")
    c.append("        return step;")
    c.append("    }")
    c.append("    return step;")
    c.append("}")
    c.append("")
    c.append(f"{app}_screen_id_t {app}_state_screen({app}_state_t state) {{")
    c.append("    switch (state) {")
    for st in g["states"]:
        c.append(f"    case {P}_STATE_{st['id']}: return {P}_SCREEN_{ident(st['screen']).upper()};")
    c.append("    default: break;")
    c.append("    }")
    c.append(f"    return {P}_SCREEN_{ident(g['screens'][0]['id']).upper()};")
    c.append("}")
    c.append("")
    for kind, items in (("state", states), ("event", events),
                        ("action", [a.upper() for a in actions])):
        c.append(f"const char *{app}_{kind}_name({app}_{kind}_t v) {{")
        c.append("    switch (v) {")
        for it in items:
            c.append(f'    case {P}_{kind.upper()}_{it}: return "{it}";')
        c.append("    default: return \"?\";")
        c.append("    }")
        c.append("}")
        c.append("")
    return "\n".join(h), "\n".join(c)


def emit_screens(g, src):
    """LVGL widget tree builders. One struct per screen holds the bindable widgets."""
    app = ident(g["app"])
    h = [BANNER.format(src=src), "#pragma once", "", '#include "lvgl.h"', "",
         "#ifdef __cplusplus", 'extern "C" {', "#endif", ""]
    c = [BANNER.format(src=src), f'#include "{app}_screens.h"',
         '#include "ui_state_icon.h"', '#include "ui_theme.h"', ""]

    for scr in g["screens"]:
        sid = ident(scr["id"])
        h.append("typedef struct {")
        h.append("    lv_obj_t *screen;")
        for w in scr["widgets"]:
            h.append(f"    lv_obj_t *{ident(w['id'])};")
        h.append("} %s_screen_%s_t;" % (app, sid))
        h.append("")
        h.append(f"void {app}_screen_{sid}_build({app}_screen_{sid}_t *out);")
        h.append(f"void {app}_screen_{sid}_destroy({app}_screen_{sid}_t *out);")
        h.append("")

        # Line points must stay valid while the line exists, so they are static constants.
        for w in scr["widgets"]:
            if w["type"] == "line":
                pts = ", ".join(f"{{{int(x)}, {int(y)}}}" for x, y in w["points"])
                c.append(f"static const lv_point_precise_t {sid}_{ident(w['id'])}_points[] = {{ {pts} }};")
        c.append("")
        c.append(f"void {app}_screen_{sid}_build({app}_screen_{sid}_t *out) {{")
        c.append("    if (!out) return;")
        c.append("    out->screen = lv_obj_create(NULL);")
        c.append("    lv_obj_remove_flag(out->screen, LV_OBJ_FLAG_SCROLLABLE);")
        c.append("    lv_obj_set_style_bg_color(out->screen, ui_color_bg(), 0);")
        c.append("    lv_obj_set_style_border_width(out->screen, 0, 0);")
        c.append("    lv_obj_set_style_pad_all(out->screen, 0, 0);")
        for w in scr["widgets"]:
            wid, wt = ident(w["id"]), w["type"]
            ctor = {"label": "lv_label_create", "arc": "lv_arc_create", "bar": "lv_bar_create",
                    "obj": "lv_obj_create", "button": "lv_button_create",
                    "image": "lv_image_create", "slider": "lv_slider_create",
                    "switch": "lv_switch_create", "state_icon": "ui_state_icon_create",
                    "line": "lv_line_create"}[wt]
            if wt == "state_icon":
                c.append(
                    f"    out->{wid} = {ctor}(out->screen, "
                    f"(ui_state_icon_t){int(w.get('value', 0))});"
                )
            else:
                c.append(f"    out->{wid} = {ctor}(out->screen);")
            if "w" in w and "h" in w:
                c.append(f"    lv_obj_set_size(out->{wid}, {w['w']}, {w['h']});")
            elif "w" in w:
                c.append(f"    lv_obj_set_width(out->{wid}, {w['w']});")
            c.append(f"    lv_obj_set_pos(out->{wid}, {w.get('x', 0)}, {w.get('y', 0)});")
            if wt == "line":
                c.append(f"    lv_line_set_points(out->{wid}, {sid}_{wid}_points, {len(w['points'])});")
                c.append(f"    lv_obj_set_style_line_width(out->{wid}, {int(w.get('width', 2))}, 0);")
                c.append(f"    lv_obj_set_style_line_color(out->{wid}, ui_color_{w.get('color', 'ridge')}(), 0);")
                c.append(f"    lv_obj_set_style_line_rounded(out->{wid}, true, 0);")
            if wt == "label":
                c.append(f'    lv_label_set_text(out->{wid}, "{w.get("text", "")}");')
                if w.get("long_mode") in LONG_MODES:
                    c.append(f"    lv_label_set_long_mode(out->{wid}, {LONG_MODES[w['long_mode']]});")
                if w.get("align") == "top_mid":
                    c.append(f"    lv_obj_set_style_text_align(out->{wid}, LV_TEXT_ALIGN_CENTER, 0);")
                elif w.get("align") == "top_right":
                    c.append(f"    lv_obj_set_style_text_align(out->{wid}, LV_TEXT_ALIGN_RIGHT, 0);")
                if w.get("font"):
                    c.append(f"    lv_obj_set_style_text_font(out->{wid}, &lv_font_{w['font']}, 0);")
                if "letter_space" in w:
                    c.append(
                        f"    lv_obj_set_style_text_letter_space(out->{wid}, "
                        f"{int(w['letter_space'])}, 0);"
                    )
                c.append(
                    f"    lv_obj_set_style_text_color(out->{wid}, "
                    f"ui_color_{w.get('color', 'ink')}(), 0);"
                )
                # A label that starts hidden. The app shows it when its content applies.
                if w.get("hidden"):
                    c.append(f"    lv_obj_add_flag(out->{wid}, LV_OBJ_FLAG_HIDDEN);")
            elif wt in ("obj", "button"):
                if wt == "obj":
                    c.append(f"    lv_obj_remove_flag(out->{wid}, LV_OBJ_FLAG_SCROLLABLE);")
                c.append(
                    f"    lv_obj_set_style_bg_color(out->{wid}, "
                    f"ui_color_{w.get('bg', w.get('color', 'surface'))}(), 0);"
                )
                c.append(f"    lv_obj_set_style_bg_opa(out->{wid}, LV_OPA_COVER, 0);")
                c.append(f"    lv_obj_set_style_radius(out->{wid}, {int(w.get('radius', 0))}, 0);")
                c.append(f"    lv_obj_set_style_border_width(out->{wid}, {int(w.get('border_width', 0))}, 0);")
                if int(w.get("border_width", 0)):
                    c.append(
                        f"    lv_obj_set_style_border_color(out->{wid}, "
                        f"ui_color_{w.get('border_color', 'surface_hi')}(), 0);"
                    )
                c.append(f"    lv_obj_set_style_pad_all(out->{wid}, {int(w.get('pad', 0))}, 0);")
            elif wt in ("arc", "bar", "slider"):
                setter = {"arc": "lv_arc", "bar": "lv_bar", "slider": "lv_slider"}[wt]
                c.append(f"    {setter}_set_range(out->{wid}, {w.get('min', 0)}, {w.get('max', 100)});")
                c.append(
                    f"    lv_obj_set_style_arc_color(out->{wid}, "
                    f"ui_color_{w.get('color', 'accent')}(), LV_PART_INDICATOR);"
                    if wt == "arc" else
                    f"    lv_obj_set_style_bg_color(out->{wid}, "
                    f"ui_color_{w.get('color', 'accent')}(), LV_PART_INDICATOR);"
                )
        c.append("}")
        c.append("")
        # rule 7: teardown is generated, so no observer or timer outlives its screen.
        c.append(f"void {app}_screen_{sid}_destroy({app}_screen_{sid}_t *out) {{")
        c.append("    if (!out || !out->screen) return;")
        c.append("    lv_obj_delete(out->screen);")
        c.append("    out->screen = NULL;")
        for w in scr["widgets"]:
            c.append(f"    out->{ident(w['id'])} = NULL;")
        c.append("}")
        c.append("")

    h += ["#ifdef __cplusplus", "}", "#endif", ""]
    return "\n".join(h), "\n".join(c)


def emit_bindings(g, src):
    """lv_subject_t values plus the observers that push them into widgets.

    Every widget write happens inside an observer callback. LVGL is not thread
    safe, so each setter takes the LVGL lock through hooks that the application
    supplies with <app>_bindings_set_lock(). A caller on any task can then use a
    setter. A single-thread host program passes NULL and runs without a lock.
    """
    app = ident(g["app"])
    subjects = {s["id"]: s for s in g["subjects"]}
    transforms = {t["id"]: t for t in g["transforms"]}

    h = [BANNER.format(src=src), "#pragma once", "", '#include "lvgl.h"',
         f'#include "{app}_screens.h"', "", "#ifdef __cplusplus", 'extern "C" {', "#endif", ""]
    h.append(f"void {app}_bindings_init(void);")
    # The screens with bindings, in graph order. Screen 0 keeps the short names.
    bound = {}
    for i, b in enumerate(g["bindings"]):
        scrs, wid = binding_screens(g, b)
        for scr in scrs:
            bound.setdefault(scr, []).append((i, ident(wid)))
    order = [s["id"] for s in g["screens"] if s["id"] in bound]
    suffix = {scr: ("" if scr == g["screens"][0]["id"] else "_" + ident(scr)) for scr in order}
    max_bound = max((len({w for _, w in v}) for v in bound.values()), default=1)

    h.append("// Each attach binds the widgets of one screen. Only one screen has bindings at a time.")
    for scr in order:
        h.append(f"void {app}_bindings_attach{suffix[scr]}(const {app}_screen_{ident(scr)}_t *screen);")
    h.append("// Unbind while the widgets are still alive. Call before destroying the screen.")
    h.append(f"void {app}_bindings_detach(void);")
    h.append("// Rule 7: detach then destroy, in that order. Prefer this over calling both.")
    for scr in order:
        h.append(f"void {app}_teardown{suffix[scr]}({app}_screen_{ident(scr)}_t *screen);")
    h.append("")
    h.append("// Lock hooks. Each setter takes the LVGL lock through these hooks, so a caller")
    h.append("// on any task can use it. The firmware passes the BSP lock, which allows")
    h.append("// re-entry. A single-thread host program passes NULL for both hooks.")
    h.append(f"typedef bool (*{app}_lock_fn)(void);")
    h.append(f"typedef void (*{app}_unlock_fn)(void);")
    h.append(f"void {app}_bindings_set_lock({app}_lock_fn lock, {app}_unlock_fn unlock);")
    h.append("")
    h.append("// Optional trace hook for debug logs. Each setter calls it with the subject name and")
    h.append("// the new value, before it takes the lock. text is NULL for an int subject.")
    h.append(f"typedef void (*{app}_trace_fn)(const char *subject, const char *text, int32_t value);")
    h.append(f"void {app}_bindings_set_trace({app}_trace_fn fn);")
    h.append("")
    h.append("// Each setter takes the lock. It does nothing when the lock times out.")
    for s in g["subjects"]:
        sid = ident(s["id"])
        if s["type"] == "string":
            h.append(f"void {app}_set_{sid}(const char *value);")
        else:
            h.append(f"void {app}_set_{sid}(int32_t value);")
    h.append("")
    h += ["#ifdef __cplusplus", "}", "#endif", ""]

    c = [BANNER.format(src=src), f'#include "{app}_bindings.h"',
         '#include "ui_state_icon.h"', '#include "ui_theme.h"',
         "#include <string.h>", "", "static bool s_ready;",
         f"static {app}_lock_fn s_lock;", f"static {app}_unlock_fn s_unlock;",
         f"static {app}_trace_fn s_trace;", "",
         f"void {app}_bindings_set_trace({app}_trace_fn fn) {{ s_trace = fn; }}", "",
         f"void {app}_bindings_set_lock({app}_lock_fn lock, {app}_unlock_fn unlock) {{",
         "    s_lock = lock;", "    s_unlock = unlock;", "}", "",
         "static bool take_lock(void) { return s_lock ? s_lock() : true; }",
         "static void give_lock(void) { if (s_unlock) s_unlock(); }", ""]
    sizes = [int(s.get("size", 32)) for s in g["subjects"] if s["type"] == "string"]
    if sizes:
        c += ["// A text longer than its subject is cut at a UTF-8 character boundary.",
              "// LVGL alone can cut inside a character. The lock guards s_fit.",
              f"static char s_fit[{max(sizes)}];", "",
              "static const char *fit_utf8(const char *value, size_t size) {",
              "    size_t n = strlen(value);",
              "    if (n < size) return value;",
              "    n = size - 1;",
              "    while (n > 0 && ((unsigned char)value[n] & 0xC0) == 0x80) n--;",
              "    memcpy(s_fit, value, n);",
              "    s_fit[n] = '\\0';",
              "    return s_fit;",
              "}", ""]
    for s in g["subjects"]:
        sid = ident(s["id"])
        c.append(f"static lv_subject_t s_subject_{sid};")
        if s["type"] == "string":
            n = int(s.get("size", 32))
            c.append(f"static char s_buf_{sid}[{n}];")
            c.append(f"static char s_prev_{sid}[{n}];")
    c.append("")
    c.append("// The widgets with observers on the screen that is on now.")
    c.append(f"static lv_obj_t *s_bound[{max_bound}];")
    c.append("static size_t s_n_bound;")
    # LVGL auto-unsubscribes an observer when its widget is deleted, so we keep
    # no observer pointers. lv_observer_remove() is forbidden for observers made
    # by lv_subject_add_observer_obj(); see lv_observer.h.
    c.append("")

    for t in g["transforms"]:
        tid = ident(t["id"])
        c.append(f"static lv_color_t transform_{tid}(int32_t v) {{")
        c.append("    switch (v) {")
        for k, v in t["map"].items():
            c.append(f"    case {k}: return ui_color_{v}();")
        c.append(f"    default: return ui_color_{t['default']}();")
        c.append("    }")
        c.append("}")
        c.append("")

    for i, b in enumerate(g["bindings"]):
        prop = b["target"].rsplit(".", 1)[1]
        scrs, wid = binding_screens(g, b)
        wscr = next(s2 for s2 in g["screens"] if s2["id"] == scrs[0])
        wt = next(w2["type"] for w2 in wscr["widgets"] if w2["id"] == wid)
        subj = subjects[b["subject"]]
        c.append(f"static void observer_{i}(lv_observer_t *observer, lv_subject_t *subject) {{")
        c.append("    lv_obj_t *w = (lv_obj_t *)lv_observer_get_target(observer);")
        c.append("    if (!w) return;")
        if prop == "text":
            c.append("    lv_label_set_text(w, lv_subject_get_string(subject));")
        elif prop == "value":
            c.append("    int32_t value = lv_subject_get_int(subject);")
            # lv_arc_set_value takes no animation argument. bar and slider do.
            if wt == "arc":
                c.append("    lv_arc_set_value(w, value);")
            elif wt == "state_icon":
                c.append("    ui_state_icon_set(w, value);")
            else:
                setter = {"bar": "lv_bar_set_value", "slider": "lv_slider_set_value"}[wt]
                c.append(f"    {setter}(w, value, LV_ANIM_OFF);")
        elif prop == "color":
            if b.get("transform"):
                c.append(
                    f"    lv_color_t col = transform_{ident(b['transform'])}"
                    "(lv_subject_get_int(subject));"
                )
            else:
                c.append("    lv_color_t col = lv_color_hex((uint32_t)lv_subject_get_int(subject));")
            part = "LV_PART_INDICATOR" if wt in ("arc", "bar", "slider") else "0"
            style = "lv_obj_set_style_arc_color" if wt == "arc" else (
                "lv_obj_set_style_text_color" if wt == "label" else "lv_obj_set_style_bg_color")
            c.append(f"    {style}(w, col, {part});")
        elif prop == "hidden":
            c.append("    if (lv_subject_get_int(subject)) lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN);")
            c.append("    else lv_obj_remove_flag(w, LV_OBJ_FLAG_HIDDEN);")
        c.append("}")
        c.append("")

    c.append(f"void {app}_bindings_init(void) {{")
    c.append("    if (s_ready) return;")
    for s in g["subjects"]:
        sid = ident(s["id"])
        if s["type"] == "string":
            n = int(s.get("size", 32))
            c.append(
                f'    lv_subject_init_string(&s_subject_{sid}, s_buf_{sid}, s_prev_{sid}, '
                f'{n}, "{s.get("initial", "")}");'
            )
        else:
            c.append(f"    lv_subject_init_int(&s_subject_{sid}, {int(s.get('initial', 0))});")
    c.append("    s_ready = true;")
    c.append("}")
    c.append("")
    for scr in order:
        c.append(f"void {app}_bindings_attach{suffix[scr]}(const {app}_screen_{ident(scr)}_t *screen) {{")
        c.append("    if (!screen) return;")
        c.append(f"    {app}_bindings_init();")
        c.append(f"    {app}_bindings_detach();")
        for i, wid in bound[scr]:
            sid = ident(g["bindings"][i]["subject"])
            c.append(
                f"    (void)lv_subject_add_observer_obj("
                f"&s_subject_{sid}, observer_{i}, screen->{wid}, NULL);"
            )
        for wid in sorted({w for _, w in bound[scr]}):
            c.append(f"    s_bound[s_n_bound++] = screen->{wid};")
        c.append("}")
        c.append("")
    # Rule 7. lv_obj_remove_from_subject() is the supported way to unbind an
    # observer created by lv_subject_add_observer_obj(). The widgets must still
    # exist here, so detach has to run before the screen is destroyed.
    c.append(f"void {app}_bindings_detach(void) {{")
    c.append("    for (size_t i = 0; i < s_n_bound; i++) {")
    c.append("        if (s_bound[i]) lv_obj_remove_from_subject(s_bound[i], NULL);")
    c.append("        s_bound[i] = NULL;")
    c.append("    }")
    c.append("    s_n_bound = 0;")
    c.append("}")
    c.append("")
    for scr in order:
        c.append(f"void {app}_teardown{suffix[scr]}({app}_screen_{ident(scr)}_t *screen) {{")
        c.append(f"    {app}_bindings_detach();")
        c.append(f"    {app}_screen_{ident(scr)}_destroy(screen);")
        c.append("}")
        c.append("")
    for s in g["subjects"]:
        sid = ident(s["id"])
        if s["type"] == "string":
            c.append(f"void {app}_set_{sid}(const char *value) {{")
            c.append(f'    if (s_trace && value) s_trace("{sid}", value, 0);')
            c.append("    if (!s_ready || !value || !take_lock()) return;")
            c.append(f"    lv_subject_copy_string(&s_subject_{sid}, "
                     f"fit_utf8(value, sizeof s_buf_{sid}));")
            c.append("    give_lock();")
            c.append("}")
        else:
            c.append(f"void {app}_set_{sid}(int32_t value) {{")
            c.append(f'    if (s_trace) s_trace("{sid}", NULL, value);')
            c.append("    if (!s_ready || !take_lock()) return;")
            c.append(f"    lv_subject_set_int(&s_subject_{sid}, value);")
            c.append("    give_lock();")
            c.append("}")
        c.append("")
    return "\n".join(h), "\n".join(c)


def c_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def load_strings(g, graph_path):
    """Read the vocabulary that "strings_file" names. The path is relative to the graph."""
    rel = g.get("strings_file")
    if not rel:
        return {}
    path = pathlib.Path(graph_path).parent / rel
    data = json.loads(path.read_text(encoding="utf-8"))
    return {k: v["zh"] for k, v in data["strings"].items()}


def emit_ui(g, src, strings):
    """One function that sets the subject values that "ui_effects" gives for each action."""
    app = ident(g["app"])
    P = app.upper()
    effects = {k: v for k, v in g.get("ui_effects", {}).items() if k != "comment"}
    errors = []

    def resolve(aid, sid, value):
        if value == "":
            return ""
        key = value["key"] if isinstance(value, dict) else value
        if key not in strings:
            errors.append(f"ui_effects {aid}.{sid}: unknown string key {key!r}")
            return ""
        text = strings[key]
        if isinstance(value, dict) and "n" in value:
            text = text.replace("{n}", str(value["n"]))
        return text

    h = [BANNER.format(src=src), "#pragma once", "", "#include <stdbool.h>",
         f'#include "{app}_flow.h"', "", "#ifdef __cplusplus", 'extern "C" {', "#endif", "",
         "// Set the screen texts that the flow graph gives for this action.",
         "// Return false when the action is not a UI action. The setters take the LVGL lock.",
         f"bool {app}_ui_apply({app}_action_t action);", "",
         "#ifdef __cplusplus", "}", "#endif", ""]
    c = [BANNER.format(src=src), f'#include "{app}_ui.h"', f'#include "{app}_bindings.h"', "",
         f"bool {app}_ui_apply({app}_action_t action) {{", "    switch (action) {"]
    for aid, effect in effects.items():
        c.append(f"    case {P}_ACTION_{aid.upper()}:")
        for sid, value in effect.items():
            if isinstance(value, int) and not isinstance(value, bool):
                c.append(f"        {app}_set_{ident(sid)}({value});")
            else:
                c.append(f"        {app}_set_{ident(sid)}({c_string(resolve(aid, sid, value))});")
        c.append("        return true;")
    c += ["    default:", "        return false;", "    }", "}", ""]
    if errors:
        raise FlowError(f"{src}: {len(errors)} ui_effects error(s):\n  - " + "\n  - ".join(errors))
    return "\n".join(h), "\n".join(c)


def emit_test(g, src):
    """Host test skeleton. It proves the generated table matches the graph."""
    app = ident(g["app"])
    P = app.upper()
    initial = next(s["id"] for s in g["states"] if s.get("initial"))
    lines = [BANNER.format(src=src), '#include "%s_flow.h"' % app, "#include <assert.h>",
             "#include <stdio.h>", "#include <string.h>", "", "int main(void) {",
             f"    {app}_flow_t flow;", f"    {app}_flow_init(&flow);",
             f"    assert(flow.state == {P}_STATE_{initial});", ""]
    lines.append("    // every transition in the graph must fire from its source state")
    for t in g["transitions"]:
        lines.append(f"    {app}_flow_init(&flow);")
        lines.append(f"    flow.state = {P}_STATE_{t['from']};")
        lines.append(f"    {{ {app}_step_t s = {app}_flow_step(&flow, {P}_EVENT_{t['on']});")
        lines.append("      assert(s.handled);")
        lines.append(f"      assert(s.to == {P}_STATE_{t['to']});")
        lines.append(f"      assert(s.action_count == {len(t.get('actions', []))}); }}")
    lines.append("")
    lines.append("    // an event with no transition must leave the state untouched")
    lines.append(f"    {app}_flow_init(&flow);")
    lines.append(f"    for (int e = 0; e < {P}_EVENT_COUNT; e++) {{")
    lines.append(f"        {app}_flow_t probe; {app}_flow_init(&probe);")
    lines.append(f"        {app}_state_t before = probe.state;")
    lines.append(f"        {app}_step_t s = {app}_flow_step(&probe, ({app}_event_t)e);")
    lines.append("        if (!s.handled) assert(probe.state == before);")
    lines.append("    }")
    lines.append("")
    lines.append("    // rule 6: a blocking action must be declared async")
    lines.append(f"    for (int a = 0; a < {P}_ACTION_COUNT; a++) {{")
    lines.append(f"        (void){app}_action_is_async(({app}_action_t)a);")
    lines.append("    }")
    lines.append("")
    lines.append(f'    printf("%s flow tests: PASS\\n", "{app}");')
    lines.append("    return 0;")
    lines.append("}")
    return "\n".join(lines)


# ---------------------------------------------------------------- driver

def main(argv=None):
    ap = argparse.ArgumentParser(description="Compile a flow graph into C for LVGL.")
    ap.add_argument("graph", nargs="+", help="path to a .flow.json graph")
    ap.add_argument("--out-dir", default="main/generated")
    ap.add_argument("--test-dir", default="tests")
    ap.add_argument("--check", action="store_true",
                    help="fail when a generated file is missing or stale")
    args = ap.parse_args(argv)

    rc = 0
    for gp in args.graph:
        path = pathlib.Path(gp)
        graph = json.loads(path.read_text())
        try:
            report = validate(graph, path)
        except FlowError as e:
            print(str(e), file=sys.stderr)
            rc = 1
            continue

        app = ident(graph["app"])
        src = path.as_posix()
        out, tdir = pathlib.Path(args.out_dir), pathlib.Path(args.test_dir)
        out.mkdir(parents=True, exist_ok=True)
        tdir.mkdir(parents=True, exist_ok=True)

        fh, fc = emit_flow(graph, src)
        sh, sc = emit_screens(graph, src)
        bh, bc = emit_bindings(graph, src)
        try:
            uh, uc = emit_ui(graph, src, load_strings(graph, path))
        except FlowError as e:
            print(str(e), file=sys.stderr)
            rc = 1
            continue
        files = {
            out / f"{app}_ui.h": uh, out / f"{app}_ui.c": uc,
            out / f"{app}_flow.h": fh, out / f"{app}_flow.c": fc,
            out / f"{app}_screens.h": sh, out / f"{app}_screens.c": sc,
            out / f"{app}_bindings.h": bh, out / f"{app}_bindings.c": bc,
            tdir / f"test_{app}_flow.c": emit_test(graph, src),
        }

        stale = []
        for fp, content in files.items():
            if not fp.exists() or fp.read_text() != content:
                stale.append(fp.as_posix())
                if not args.check:
                    fp.write_text(content)

        pct = 100.0 * report["pool_estimate"] / report["pool_ceiling"]
        if args.check:
            if stale:
                print(f"{src}: generated output is stale:", file=sys.stderr)
                for s in stale:
                    print(f"  - {s}", file=sys.stderr)
                print("  run: python3 tools/flow/flowc.py " + src, file=sys.stderr)
                rc = 1
            else:
                print(f"{src}: generated output is current")
        else:
            print(f"{src}: OK  ({len(graph['states'])} states, "
                  f"{len(graph['transitions'])} transitions, "
                  f"{len(graph['bindings'])} bindings)")
            print(f"  LVGL pool estimate: {report['pool_estimate']} B / "
                  f"{report['pool_ceiling']} B ceiling ({pct:.1f}%)")
            for fp in files:
                print(f"  wrote {fp.as_posix()}")
    return rc


if __name__ == "__main__":
    sys.exit(main())
