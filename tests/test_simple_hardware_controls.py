# SPDX-License-Identifier: GPL-3.0-only
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class SimpleHardwareControlsTests(unittest.TestCase):
    def test_physical_overlay_is_immediate_page_first(self):
        perf = (ROOT / 'tools' / 'apply_keyboard_performance.py').read_text(encoding='utf-8')
        safety = (ROOT / 'tools' / 'fix_beta_input_safety.py').read_text(encoding='utf-8')
        self.assertIn('hardware-controls=immediate-page-first', perf)
        self.assertIn('advanced-modifiers=virtual-only', perf)
        self.assertNotIn('refm_mod_mask', perf)
        self.assertNotIn('RB_BEGIN_MOD', perf)
        self.assertIn('refm_felucca_key_event', perf)
        self.assertIn('RB_HIT(B_SEQ)', safety)
        self.assertIn('refm_felucca_panic(); refm_felucca_ui_action(3u)', safety)

    def test_shared_performance_model_directly_edits_active_pages(self):
        shared = (ROOT / 'firmware' / 'proto' / 'ui240_performance.h').read_text(encoding='utf-8')
        self.assertIn('ui->page == UI240_DRUM', shared)
        self.assertIn('ui->page == UI240_ACID_STEP', shared)
        self.assertIn('st->note = note;', shared)
        self.assertIn('ui240_set_drum_hit', shared)


if __name__ == '__main__':
    unittest.main()
