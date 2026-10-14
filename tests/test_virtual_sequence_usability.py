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

    def test_renderer_uses_real_pattern_window_and_rebirth_pitch_controls(self):
        s = (ROOT / "web/app/rb240_renderer_v2.js").read_text(encoding="utf-8")
        self.assertIn("Array.from({length:16}", s)
        self.assertIn("const acidSteps=packed.map(acidStep)", s)
        self.assertIn("drumSteps=packed.map", s)
        self.assertIn("REBIRTH / TB-303 STEP EDIT", s)
        self.assertIn("303 register: DOWN", s)
        self.assertIn("TB_NORMAL_LOW=60", s)
        self.assertIn("TB_MAX=84", s)
        self.assertIn("'DOWN'", s)
        self.assertIn("'UP'", s)
        self.assertIn("'PAUSE'", s)
        self.assertNotIn("'OCT-'", s[s.index("function seq"):s.index("function drums")])
        self.assertIn("window.refmRB240State=s", s)
        self.assertIn("Click lane + steps", s)
        self.assertIn("K1 lane / K2 state", s)

    def test_virtual_lcd_direct_edit_contract(self):
        usability = (ROOT / "web/app/virtual_usability.js").read_text(encoding="utf-8")
        panel = (ROOT / "web/app/virtual_fm1.js").read_text(encoding="utf-8")
        wasm = (ROOT / "web/emu/refm_wasm.c").read_text(encoding="utf-8")
        self.assertIn("TB_MIN=48", usability)
        self.assertIn("TB_NORMAL_LOW=60", usability)
        self.assertIn("TB_NORMAL_HIGH=72", usability)
        self.assertIn("TB_MAX=84", usability)
        self.assertIn("function setOctaveMode", usability)
        self.assertIn("function shiftSelectedOctave", usability)
        self.assertIn("effective303Note(base,mode)", usability)
        self.assertIn("api?.setAcidStep", usability)
        self.assertIn("api?.getDrumStep&&api?.setDrumStep", usability)
        self.assertIn("function drumLaneState", usability)
        self.assertIn("function editSettings", usability)
        self.assertIn("s.page===1||s.page===4||s.page===5", usability)
        self.assertIn("moveWrapped(s.selectedStep,target,16,'PRESETS')", usability)
        self.assertIn("moveWrapped(s.lane,target,11,'KNOB1')", usability)
        self.assertIn("setAcidStep:setAcidStepDirect", panel)
        self.assertIn("Math.max(48,Math.min(84", panel)
        self.assertIn("flags&63", panel)
        self.assertIn("shiftAcidOctave", panel)
        self.assertIn("SEQ16_DOWN|SEQ16_UP", wasm)
        self.assertIn("if(note<48u)note=48u", wasm)
        self.assertIn("if(note>84u)note=84u", wasm)
        self.assertIn("static const uint8_t a0[16]={60,60,67", wasm)

    def test_true_two_x_display_rerenders_and_visibly_resizes_inside_panel(self):
        usability = (ROOT / "web/app/virtual_usability.js").read_text(encoding="utf-8")
        boot = (ROOT / "web/app/rb240_boot_renderer.js").read_text(encoding="utf-8")
        self.assertIn("const px=display2x?480:240", usability)
        self.assertIn("if(canvas.width!==px)canvas.width=px", usability)
        self.assertIn("if(canvas.height!==px)canvas.height=px", usability)
        self.assertIn("device?.classList.toggle('display2x',display2x)", usability)
        self.assertIn(".virtualDevice.display2x>#virtualScreen", usability)
        self.assertIn("width:40%", usability)
        self.assertIn("LCD 480 × 480 · LARGE PANEL", usability)
        self.assertNotIn("drawImage(src,0,0,480,480)", usability)
        self.assertNotIn("zoomCanvas", usability)
        self.assertIn("const sx=w/240,sy=h/240", boot)
        self.assertIn("g.setTransform(sx,0,0,sy,0,0)", boot)
        self.assertIn("four times as many pixels", boot)

    def test_physical_overlay_keeps_real_keyboard_while_virtual_keeps_rebirth_register(self):
        stack = (ROOT / "tools" / "patch_fm1_rebirth_ui.py").read_text(encoding="utf-8")
        patch = (ROOT / "tools" / "apply_rebirth303_pitch_model.py").read_text(encoding="utf-8")
        self.assertLess(stack.index("apply_hardware_rework.py"), stack.index("apply_rebirth303_pitch_model.py"))
        self.assertIn("hardware has a real 27-key chromatic keyboard", patch)
        self.assertIn("musical keys: enter exactly the pitch", patch)
        self.assertIn("song.octave > -3", patch)
        self.assertIn("song.octave < 3", patch)
        self.assertIn("keyboard OCT-", patch)
        self.assertIn("keyboard OCT+", patch)
        self.assertIn("hardware-step-entry=exact-midi-note", patch)
        self.assertIn("virtual-step-keyboard=rebirth-c-to-c-down-normal-up", patch)
        self.assertNotIn("void refm_felucca_step_octave(int8_t direction)", patch)
        self.assertNotIn("refm_felucca_step_octave(-1)", patch)
        self.assertNotIn("refm_felucca_step_octave(1)", patch)

    def test_compatibility_entry_preserves_boot_then_v2_chain(self):
        entry = (ROOT / "web/app/rb240_renderer.js").read_text(encoding="utf-8")
        boot = (ROOT / "web/app/rb240_boot_renderer.js").read_text(encoding="utf-8")
        self.assertIn("rb240_boot_renderer.js", entry)
        self.assertIn("rb240_renderer_v2.js", boot)


if __name__ == "__main__":
    unittest.main()
