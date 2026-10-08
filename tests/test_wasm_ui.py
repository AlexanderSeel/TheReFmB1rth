import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JS = (ROOT / 'web/emu/rebirth_skin.js').read_text(encoding='utf-8')
SAMPLE = (ROOT / 'web/emu/sample_controls.js').read_text(encoding='utf-8')
CSS = (ROOT / 'web/emu/rebirth_skin.css').read_text(encoding='utf-8')
BUILD = (ROOT / 'web/emu/build_wasm.sh').read_text(encoding='utf-8')
INDEX = (ROOT / 'web/emu/index.html').read_text(encoding='utf-8')
WASM_C = (ROOT / 'web/emu/refm_wasm.c').read_text(encoding='utf-8')


class WasmUiContractTests(unittest.TestCase):
    def test_rotary_knobs_are_interactive(self):
        for token in ('pointerdown', 'pointermove', 'wheel', 'dblclick', 'ArrowUp', 'refm-knob-pointer'):
            self.assertIn(token, JS)
        self.assertIn('.refm-knob{', CSS)
        self.assertIn('--angle', JS)

    def test_sequence_json_round_trip_ui_exists(self):
        for token in ('SAVE JSON', 'LOAD JSON', 'TheReFmB1rth.sequence', 'application/json', '_refm_wasm_set_acid_step', '_refm_wasm_set_drum_step'):
            self.assertIn(token, JS)
        self.assertIn('.refm-sequence-tools', CSS)

    def test_rack_pattern_and_step_tools_exist(self):
        for token in ('data-pattern', 'data-prob', 'data-copy', 'data-paste', 'refm:patternchange'):
            self.assertIn(token, JS)
        self.assertIn('.refm-pattern-tools', CSS)
        self.assertIn('.refm-prob', CSS)

    def test_303_time_mode_is_note_rest_tie(self):
        for token in ('data-time="note"', 'data-time="rest"', 'data-time="tie"', 'TIME MODE', "s.flags=TIE", "s.flags=0"):
            self.assertIn(token, SAMPLE)
        self.assertIn("(s.flags&(ACCENT|SLIDE))|GATE", SAMPLE)
        self.assertNotIn('data-time="gate"', SAMPLE)
        # Accent and slide are note modifiers, not alternate timing types.
        self.assertIn('data-modifier="2"', SAMPLE)
        self.assertIn('data-modifier="4"', SAMPLE)

    def test_engine_state_drives_mixer_and_tempo(self):
        self.assertIn('_refm_wasm_get_fx', JS)
        self.assertIn('_refm_wasm_set_bpm', INDEX)
        self.assertNotIn("q('#bpm').oninput=e=>{q('#bpmRead').textContent=e.target.value;mod._refm_wasm_init", INDEX)
        self.assertIn('refm_wasm_set_bpm', WASM_C)
        self.assertIn('refm_wasm_get_fx', WASM_C)

    def test_skin_is_packaged(self):
        self.assertIn('cp "$ROOT/web/emu/rebirth_skin.css" "$OUT/rebirth_skin.css"', BUILD)
        self.assertIn('cp "$ROOT/web/emu/rebirth_skin.js" "$OUT/rebirth_skin.js"', BUILD)
        self.assertIn('cp "$ROOT/web/emu/sample_controls.js" "$OUT/sample_controls.js"', BUILD)


if __name__ == '__main__':
    unittest.main()
