# SPDX-License-Identifier: GPL-3.0-only
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class HardwareReworkTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.patch = (ROOT / 'tools' / 'apply_hardware_rework.py').read_text(encoding='utf-8')
        cls.overlay = (ROOT / 'tools' / 'patch_fm1_rebirth_ui.py').read_text(encoding='utf-8')

    def test_keyboard_is_scanned_from_live_target_audio_path(self):
        self.assertIn('static void refm_hw_keyboard_scan(void)', self.patch)
        self.assertIn('uint32_t cur = fm1_in.notes;', self.patch)
        self.assertIn('static uint8_t refm_hw_note[27];', self.patch)
        self.assertIn('refm_felucca_key_event((uint8_t)k, refm_hw_note[k], 1u)', self.patch)
        self.assertIn('refm_felucca_key_event((uint8_t)k, refm_hw_note[k], 0u)', self.patch)
        self.assertIn('refm_hw_keyboard_scan();\\n    refm_felucca_audio_block(out, n);', self.patch)

    def test_physical_master_and_safe_headroom_are_in_target_output_path(self):
        self.assertIn('#define REFM_HW_HEADROOM_Q15 16384', self.patch)
        self.assertIn('song.master_q12', self.patch)
        self.assertIn('fx_usb_fixed ? 4096', self.patch)
        self.assertIn('refm_hw_output_gain(out, n);', self.patch)

    def test_buttons_and_machine_selection_are_contextual(self):
        self.assertIn('static void cycle_machine_context(int8_t delta)', self.patch)
        self.assertIn('g_ui.page == UI240_DRUM && g_ui.machine == UI240_MACHINE_808', self.patch)
        self.assertIn('g_ui.page == UI240_DRUM && g_ui.machine == UI240_MACHINE_909', self.patch)
        self.assertIn('if (g_ui.machine >= UI240_MACHINE_808) ui240_set_machine(&g_ui, UI240_MACHINE_ACID1);', self.patch)
        self.assertIn('else if (song.octave > -3)', self.patch)
        self.assertIn('else if (song.octave < 3)', self.patch)

    def test_renderer_is_compact_and_dirty_region_driven(self):
        self.assertIn('struct rb5_cache', self.patch)
        self.assertIn('if (force) lcd_fill(0, 0, 240, 240, RB5_BG);', self.patch)
        self.assertIn('force || p0 != rb5.p0', self.patch)
        self.assertIn('force || packed != rb5.step[i]', self.patch)
        self.assertIn('refm_felucca_pattern_step((uint8_t)i)', self.patch)
        self.assertNotIn('PRESET=PAT  ALGO=TRACK  SELECT=BPM', self.patch)
        self.assertNotIn('Pattern playback / chain focus', self.patch)

    def test_final_pass_is_last_in_overlay_stack(self):
        boot = self.overlay.index('apply_refm_boot_splash.py')
        final = self.overlay.index('apply_hardware_rework.py')
        hashing = self.overlay.index('h = hashlib.sha256()')
        self.assertLess(boot, final)
        self.assertLess(final, hashing)


if __name__ == '__main__':
    unittest.main()
