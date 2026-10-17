#!/usr/bin/env python3
"""Negative tests for the flow validator.

Each case breaks one rule in a known-good graph and asserts that flowc rejects
it. A validator that never says no is worth nothing, so these matter more than
the happy path.
"""
import copy
import json
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "flow"))

import flowc  # noqa: E402

GOOD = json.loads((ROOT / "flows" / "coach.flow.json").read_text())


def check(graph):
    return flowc.validate(graph, "test.flow.json")


class ValidatorTest(unittest.TestCase):
    def assertRejects(self, graph, needle):
        with self.assertRaises(flowc.FlowError) as ctx:
            check(graph)
        self.assertIn(needle, str(ctx.exception))

    def test_good_graph_passes(self):
        report = check(copy.deepcopy(GOOD))
        self.assertLess(report["pool_estimate"], report["pool_ceiling"])

    def test_rule1_unreachable_state(self):
        g = copy.deepcopy(GOOD)
        g["states"].append({"id": "ORPHAN", "screen": "coach"})
        g["transitions"].append({"from": "ORPHAN", "on": "LINK_UP", "to": "READY"})
        self.assertRejects(g, "rule 1")

    def test_rule2_no_route_back_to_root(self):
        g = copy.deepcopy(GOOD)
        g["states"].append({"id": "TRAP", "screen": "coach"})
        g["transitions"].append({"from": "READY", "on": "LINK_UP", "to": "TRAP"})
        self.assertRejects(g, "rule 2")

    def test_rule3_ok_long_without_replacement_back(self):
        g = copy.deepcopy(GOOD)
        g["input"]["back_intents"] = []
        self.assertRejects(g, "rule 3")

    def test_rule4_type_mismatch(self):
        g = copy.deepcopy(GOOD)
        # drive a text property from an int subject
        g["bindings"].append({"subject": "link_state", "target": "status.text"})
        self.assertRejects(g, "rule 4")

    def test_rule4_property_not_on_widget(self):
        g = copy.deepcopy(GOOD)
        g["bindings"].append({"subject": "link_state", "target": "status.value"})
        self.assertRejects(g, "rule 4")

    def test_rule4_unknown_subject(self):
        g = copy.deepcopy(GOOD)
        g["bindings"].append({"subject": "nope", "target": "status.text"})
        self.assertRejects(g, "rule 4")

    def test_rule4_transform_input_type_mismatch(self):
        g = copy.deepcopy(GOOD)
        g["transforms"][0]["from"] = "string"
        self.assertRejects(g, "rule 4")

    def test_rule5_pool_ceiling_exceeded(self):
        g = copy.deepcopy(GOOD)
        widgets = g["screens"][0]["widgets"]
        for i in range(200):
            widgets.append({"id": f"filler{i}", "type": "label", "x": 0, "y": 0,
                            "w": 10, "text": "x"})
        self.assertRejects(g, "rule 5")

    def test_rule6_action_missing_async_flag(self):
        g = copy.deepcopy(GOOD)
        del g["actions"][0]["async"]
        self.assertRejects(g, "rule 6")

    def test_rule7_screen_never_shown(self):
        g = copy.deepcopy(GOOD)
        g["screens"].append({"id": "ghost", "widgets": []})
        self.assertRejects(g, "rule 7")

    def test_unknown_action_reference(self):
        g = copy.deepcopy(GOOD)
        g["transitions"][0]["actions"] = ["does_not_exist"]
        self.assertRejects(g, "unknown action")

    def test_unknown_target_state(self):
        g = copy.deepcopy(GOOD)
        g["transitions"][0]["to"] = "NOWHERE"
        self.assertRejects(g, "unknown target state")

    def test_requires_exactly_one_initial_state(self):
        g = copy.deepcopy(GOOD)
        g["states"][1]["initial"] = True
        self.assertRejects(g, "exactly one initial state")

    def test_unknown_widget_type(self):
        g = copy.deepcopy(GOOD)
        g["screens"][0]["widgets"][0]["type"] = "hologram"
        self.assertRejects(g, "unknown type")

    def test_errors_are_collected_not_short_circuited(self):
        g = copy.deepcopy(GOOD)
        g["screens"].append({"id": "ghost", "widgets": []})
        del g["actions"][0]["async"]
        with self.assertRaises(flowc.FlowError) as ctx:
            check(g)
        message = str(ctx.exception)
        self.assertIn("rule 6", message)
        self.assertIn("rule 7", message)


class GeneratorTest(unittest.TestCase):
    def test_generated_flow_is_deterministic(self):
        a = flowc.emit_flow(copy.deepcopy(GOOD), "x.json")
        b = flowc.emit_flow(copy.deepcopy(GOOD), "x.json")
        self.assertEqual(a, b)

    def test_async_table_matches_graph(self):
        _, c = flowc.emit_flow(copy.deepcopy(GOOD), "x.json")
        expected = [str(bool(a["async"])).lower() for a in GOOD["actions"]]
        body = c.split("static const bool ACTION_ASYNC[] = {")[1].split("};")[0]
        got = [line.strip().rstrip(",") for line in body.strip().splitlines()]
        self.assertEqual(got, expected)

    def test_ui_effect_names_must_exist(self):
        g = copy.deepcopy(GOOD)
        g["ui_effects"]["ui_show_rest"]["no_such_subject"] = "state.ready"
        g["ui_effects"]["ui_show_nothing"] = {}
        with self.assertRaises(flowc.FlowError) as ctx:
            check(g)
        self.assertIn("unknown subject 'no_such_subject'", str(ctx.exception))
        self.assertIn("unknown action 'ui_show_nothing'", str(ctx.exception))

    def test_long_mode_dots_needs_height(self):
        g = copy.deepcopy(GOOD)
        body = next(w for w in g["screens"][0]["widgets"] if w["id"] == "body")
        del body["h"]
        with self.assertRaises(flowc.FlowError) as ctx:
            check(g)
        self.assertIn('long_mode \'dots\' needs "h"', str(ctx.exception))

    def test_unknown_long_mode_is_rejected(self):
        g = copy.deepcopy(GOOD)
        g["screens"][0]["widgets"][0]["long_mode"] = "marquee"
        with self.assertRaises(flowc.FlowError) as ctx:
            check(g)
        self.assertIn("unknown long_mode 'marquee'", str(ctx.exception))

    def test_setters_call_the_trace_hook(self):
        h, c = flowc.emit_bindings(copy.deepcopy(GOOD), "x.json")
        self.assertIn("coach_bindings_set_trace", h)
        for s in GOOD["subjects"]:
            body = c.split(f"coach_set_{s['id']}(")[1].split("}")[0]
            arg = "value, 0" if s["type"] == "string" else "NULL, value"
            self.assertIn(f's_trace("{s["id"]}", {arg})', body)
            # The trace runs before the lock, so a log line never holds the LVGL lock.
            self.assertLess(body.index("s_trace("), body.index("take_lock()"))

    def test_setters_take_the_lock(self):
        h, c = flowc.emit_bindings(copy.deepcopy(GOOD), "x.json")
        self.assertNotIn("lv_async_call", h + c)
        self.assertIn("coach_bindings_set_lock", h)
        for s in GOOD["subjects"]:
            body = c.split(f"coach_set_{s['id']}(")[1].split("}")[0]
            self.assertIn("take_lock()", body)
            self.assertIn("give_lock();", body)

    def test_state_icon_uses_the_shared_component(self):
        _, screens = flowc.emit_screens(copy.deepcopy(GOOD), "x.json")
        _, bindings = flowc.emit_bindings(copy.deepcopy(GOOD), "x.json")
        self.assertIn("ui_state_icon_create", screens)
        self.assertIn("ui_state_icon_set", bindings)

    def test_every_transition_appears_in_table(self):
        _, c = flowc.emit_flow(copy.deepcopy(GOOD), "x.json")
        for t in GOOD["transitions"]:
            self.assertIn(f"COACH_STATE_{t['from']}, COACH_EVENT_{t['on']}", c)


if __name__ == "__main__":
    unittest.main(verbosity=2)
