import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JS = (ROOT / 'web/emu/rebirth_skin.js').read_text(encoding='utf-8')
CSS = (ROOT / 'web/emu/rebirth_skin.css').read_text(encoding='utf-8')
BUILD = (ROOT / 'web/emu/build_wasm.sh').read_text(encoding='utf-8')


class WasmUiContractTests(unittest.TestCase):
    def test_rotary_knobs_are_interactive(self):
        for token in ('pointerdown', 'pointermove', 'wheel', 'dblclick', 'ArrowUp', 'refm-knob-pointer'):
            self.assertIn(token, JS)
        self.assertIn('.refm-knob{', CSS)
        self.assertIn('--angle', JS)

    def test_sequence_json_round_trip_ui_exists(self):
        for token in ('SAVE SEQUENCE', 'LOAD JSON', 'TheReFmB1rth.sequence', 'application/json', '_refm_wasm_set_acid_step', '_refm_wasm_set_drum_step'):
            self.assertIn(token, JS)
        self.assertIn('.refm-sequence-tools', CSS)

    def test_skin_is_packaged(self):
        self.assertIn('cp "$ROOT/web/emu/rebirth_skin.css" "$OUT/rebirth_skin.css"', BUILD)
        self.assertIn('cp "$ROOT/web/emu/rebirth_skin.js" "$OUT/rebirth_skin.js"', BUILD)


if __name__ == '__main__':
    unittest.main()
