# SPDX-License-Identifier: GPL-3.0-only
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class PatternEditPersistenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.patch = (ROOT / "tools" / "apply_pattern_edit_persistence.py").read_text(encoding="utf-8")
        cls.stack = (ROOT / "tools" / "patch_fm1_rebirth_ui.py").read_text(encoding="utf-8")

    def test_k1_step_edits_refresh_current_pattern_bank(self):
        self.assertIn("g_ui.page == UI240_ACID_STEP", self.patch)
        self.assertIn("ui240_edit_acid_step(&g_ui, &g_refm.groovebox, encoder, d);", self.patch)
        self.assertIn("groovebox_pattern_store(&g_refm.groovebox.patterns, &g_refm.groovebox", self.patch)
        self.assertIn("g_refm.groovebox.current_pattern", self.patch)

    def test_keyboard_and_drum_edits_are_write_through_too(self):
        self.assertIn("r = ui240_performance_key(&g_ui, &g_refm.groovebox, g_modifier, key, note, on);", self.patch)
        self.assertIn("if (on && (g_ui.page == UI240_ACID_STEP", self.patch)
        self.assertIn("g_modifier == UI240_MOD_SEQ", self.patch)
        self.assertIn("g_modifier == UI240_MOD_LIVE", self.patch)
        self.assertIn("g_modifier == UI240_MOD_DRUM", self.patch)
        self.assertIn("ui240_edit_drum(&g_ui, &g_refm.groovebox, encoder, d);", self.patch)

    def test_context_actions_refresh_pattern_bank(self):
        self.assertIn("action == REFM_ACT_CONTEXT || action == REFM_ACT_ALT", self.patch)

    def test_fix_runs_after_hardware_rework_and_before_target_fixers(self):
        rework = self.stack.index("apply_hardware_rework.py")
        persistence = self.stack.index("apply_pattern_edit_persistence.py")
        gain = self.stack.index("fix_hardware_gain_target.py")
        self.assertLess(rework, persistence)
        self.assertLess(persistence, gain)


if __name__ == "__main__":
    unittest.main()
