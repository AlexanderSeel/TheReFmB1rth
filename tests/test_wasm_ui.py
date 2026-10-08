import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JS = (ROOT / 'web/emu/rebirth_skin.js').read_text(encoding='utf-8')
CSS = (ROOT / 'web/emu/rebirth_skin.css').read_text(encoding='utf-8')
BUILD = (ROOT / 'web/emu/build_wasm.sh').read_text(encoding='utf-8')
INDEX = (ROOT / 'web/emu/index.html').read_text(encoding='utf-8')


class WasmUiContractTests(unittest.TestCase):
    def test_rotary_knobs_are_interactive(self):
        for token in ('pointerdown', 'pointermove', 'wheel', 'dblclick', 'ArrowUp', 'refm-knob-pointer'):
            self.assertIn(token, JS)
        for token in ('.refm-knob{', '.refm-knob-pointer', '.refm-knob-value'):
            self.assertIn(token, CSS)
        self.assertIn('--angle', JS)
        self.assertIn("Shift=fine", JS)

    def test_sequence_json_round_trip_ui_exists(self):
        for token in ('SAVE JSON', 'LOAD JSON', 'TheReFmB1rth.sequence', 'application/json', '_refm_wasm_set_acid_step', '_refm_wasm_set_drum_step'):
            self.assertIn(token, JS)
        for token in ('.refm-sequence-tools', '.refm-pattern-tools', 'data-pattern', 'data-prob', 'data-copy', 'data-paste'):
            self.assertTrue(token in CSS or token in JS, token)

    def test_tempo_changes_do_not_reset_the_engine(self):
        self.assertIn('_refm_wasm_set_bpm', BUILD)
        self.assertIn('mod._refm_wasm_set_bpm(bpm)', INDEX)
        self.assertNotIn("q('#bpm').oninput=e=>{q('#bpmRead').textContent=e.target.value;mod._refm_wasm_init", INDEX)

    def test_fx_state_is_read_back_for_real_knob_positions(self):
        self.assertIn('_refm_wasm_get_fx', BUILD)
        self.assertIn('_refm_wasm_get_fx(param)', JS)
        self.assertIn('_refm_wasm_get_mix_track(track,0)', JS)

    def test_skin_is_packaged(self):
        self.assertIn('cp "$ROOT/web/emu/rebirth_skin.css" "$OUT/rebirth_skin.css"', BUILD)
        self.assertIn('cp "$ROOT/web/emu/rebirth_skin.js" "$OUT/rebirth_skin.js"', BUILD)


if __name__ == '__main__':
    unittest.main()
