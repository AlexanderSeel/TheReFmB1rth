# SPDX-License-Identifier: GPL-3.0-only
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class VirtualSequenceUsabilityTests(unittest.TestCase):
    def test_wasm_exposes_all_sixteen_visible_steps(self):
        s = (ROOT / "web/emu/refm_wasm_performance.inc").read_text(encoding="utf-8")
        self.assertIn("field >= 36u && field < 52u", s)
        self.assertIn("panel_sequence_step", s)
        self.assertIn("acid_seq[panel_ui.machine].step[step]", s)
        self.assertIn("drum_accents[d][step]", s)
        self.assertIn("drum_909_flams[step]", s)

    def test_renderer_uses_real_pattern_window(self):
        s = (ROOT / "web/app/rb240_renderer_v2.js").read_text(encoding="utf-8")
        self.assertIn("Array.from({length:16}", s)
        self.assertIn("const acidSteps=packed.map(acidStep)", s)
        self.assertIn("drumSteps=packed.map", s)
        self.assertIn("TB-303 STYLE STEP EDIT", s)
        self.assertIn("window.refmRB240State=s", s)
        self.assertIn("OFF → HIT → ACCENT", s)

    def test_virtual_lcd_direct_edit_contract(self):
        s = (ROOT / "web/app/virtual_usability.js").read_text(encoding="utf-8")
        self.assertIn("function editAcid", s)
        self.assertIn("function editDrum", s)
        self.assertIn("cc(track,103,note);cc(track,104,1)", s)
        self.assertIn("cc(s.machine,105,next)", s)
        self.assertIn("moveWrapped(s.selectedStep,target,16,'PRESETS')", s)
        self.assertIn("moveWrapped(s.lane,target,11,'KNOB1')", s)

    def test_true_two_x_display_rerenders_primary_panel_canvas_at_480_square(self):
        usability = (ROOT / "web/app/virtual_usability.js").read_text(encoding="utf-8")
        boot = (ROOT / "web/app/rb240_boot_renderer.js").read_text(encoding="utf-8")
        self.assertIn("const px=display2x?480:240", usability)
        self.assertIn("if(canvas.width!==px)canvas.width=px", usability)
        self.assertIn("if(canvas.height!==px)canvas.height=px", usability)
        self.assertIn("window.refmVirtual?.draw?.()", usability)
        self.assertIn("LCD 480 × 480 · HIGH DENSITY", usability)
        self.assertNotIn("drawImage(src,0,0,480,480)", usability)
        self.assertNotIn("zoomCanvas", usability)
        self.assertIn("const sx=w/240,sy=h/240", boot)
        self.assertIn("g.setTransform(sx,0,0,sy,0,0)", boot)
        self.assertIn("four times as many pixels", boot)

    def test_compatibility_entry_preserves_boot_then_v2_chain(self):
        entry = (ROOT / "web/app/rb240_renderer.js").read_text(encoding="utf-8")
        boot = (ROOT / "web/app/rb240_boot_renderer.js").read_text(encoding="utf-8")
        self.assertIn("rb240_boot_renderer.js", entry)
        self.assertIn("rb240_renderer_v2.js", boot)


if __name__ == "__main__":
    unittest.main()
