# SPDX-License-Identifier: GPL-3.0-only
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "web" / "app" / "virtual_fm1.js"
PATCHER = ROOT / "tools" / "patch_virtual_fm1_performance.py"


class VirtualInteractionContractTests(unittest.TestCase):
    def setUp(self):
        self.source = SOURCE.read_text(encoding="utf-8")

    def test_performance_abi_is_canonical_source(self):
        required = (
            "const MOD={NONE:0,SEQ:1,DRUM:2,MIXER:3,FX:4,PATTERN:5,EDIT:6,LIVE:7,FOCUS:8}",
            "const BUTTON_MOD={SEQ:1,ARP:2,GLO:3,FX:4,SAVE:5,EDIT:6,REC:7,SEL:8}",
            "_refm_wasm_panel_performance_value",
            "_refm_wasm_panel_performance_action",
            "_refm_wasm_panel_performance_encoder",
            "_refm_wasm_panel_key_event",
            "_refm_wasm_panel_modifier",
        )
        for marker in required:
            self.assertIn(marker, self.source)

    def test_modifier_primary_action_is_deferred_until_release(self):
        self.assertIn("if(BUTTON_MOD[name]!==undefined){heldModifierButton=name", self.source)
        self.assertNotIn("g.classList.add('held');fire();", self.source)
        self.assertIn("const used=modifierUsed,tap=!cancel&&!used", self.source)
        self.assertIn("if(tap)fire()", self.source)

    def test_sel_tap_clears_temporary_focus_but_consumed_hold_keeps_it(self):
        self.assertIn("function focusExit()", self.source)
        self.assertIn("if(name==='SEL'&&!used)focusExit()", self.source)
        # There must be no unconditional focus exit on every SEL release; a
        # consumed SEL+key/encoder gesture intentionally leaves FOCUS active.
        self.assertNotIn("if(name==='SEL')focusExit()", self.source)

    def test_key_or_encoder_consumes_modifier(self):
        self.assertIn("if(heldModifierButton)modifierUsed=true", self.source)
        self.assertIn("if(on&&heldModifierButton)modifierUsed=true", self.source)

    def test_blur_releases_virtual_modifier(self):
        self.assertIn("modifier(MOD.NONE);heldModifierButton=null;modifierUsed=false", self.source)

    def test_compatibility_patcher_is_idempotent_for_current_source(self):
        with tempfile.TemporaryDirectory() as td:
            copy = Path(td) / "virtual_fm1.js"
            copy.write_text(self.source, encoding="utf-8")
            before = copy.read_bytes()
            subprocess.run(["python3", str(PATCHER), str(copy)], check=True, cwd=ROOT)
            self.assertEqual(before, copy.read_bytes())


if __name__ == "__main__":
    unittest.main()
