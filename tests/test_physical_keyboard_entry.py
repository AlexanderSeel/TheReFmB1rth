# SPDX-License-Identifier: GPL-3.0-only
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class PhysicalKeyboardEntryTests(unittest.TestCase):
    def test_refm_keyboard_does_not_depend_on_hidden_felucca_track_map(self):
        patch = (ROOT / 'tools' / 'apply_keyboard_performance.py').read_text(encoding='utf-8')
        self.assertIn('53 + (int32_t)k + 12 * (int32_t)song.octave', patch)
        self.assertIn('refm_felucca_key_event((uint8_t)k, (uint8_t)note, on)', patch)
        replacement = patch.split('int32_t refm_note =', 1)[1]
        self.assertNotIn('kb_map(TSEL, k)', replacement)

    def test_oct_buttons_are_direct_editor_up_down_controls(self):
        patch = (ROOT / 'tools' / 'apply_hardware_step_navigation.py').read_text(encoding='utf-8')
        overlay = (ROOT / 'tools' / 'patch_fm1_rebirth_ui.py').read_text(encoding='utf-8')
        self.assertIn('page == 2u) refm_felucca_ui_encoder(0u, -1)', patch)
        self.assertIn('page == 2u) refm_felucca_ui_encoder(0u, 1)', patch)
        self.assertIn('page == 3u) refm_felucca_ui_encoder(0u, -1)', patch)
        self.assertIn('page == 3u) refm_felucca_ui_encoder(0u, 1)', patch)
        self.assertIn('step-select=PRESET', patch)
        self.assertIn('drum-step-select=PRESET', patch)
        self.assertIn('apply_hardware_step_navigation.py', overlay)
        self.assertIn('direct-step-navigation', overlay)

    def test_final_target_pitch_pass_restores_real_hardware_keyboard_octaves(self):
        pitch = (ROOT / 'tools' / 'apply_rebirth303_pitch_model.py').read_text(encoding='utf-8')
        overlay = (ROOT / 'tools' / 'patch_fm1_rebirth_ui.py').read_text(encoding='utf-8')
        self.assertIn('hardware-keyboard=F3..G5+octave', pitch)
        self.assertIn('hardware-step-entry=exact-midi-note', pitch)
        self.assertIn('page == 2u && song.octave > -3', pitch)
        self.assertIn('page == 2u && song.octave < 3', pitch)
        self.assertNotIn('refm_felucca_step_octave', pitch)
        self.assertIn('physical-chromatic-step-entry', overlay)


if __name__ == '__main__':
    unittest.main()
