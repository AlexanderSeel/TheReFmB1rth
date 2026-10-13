# SPDX-License-Identifier: GPL-3.0-only
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class PhysicalPatternReadbackTests(unittest.TestCase):
    def test_bridge_exposes_exact_current_machine_pattern_window(self):
        bridge = (ROOT / 'firmware' / 'integration' / 'refm_felucca_bridge.c').read_text(encoding='utf-8')
        self.assertIn('uint32_t refm_felucca_pattern_step(uint8_t step)', bridge)
        self.assertIn('st->probability << 16', bridge)
        self.assertIn('drum_909_flams[step]', bridge)
        self.assertIn('g_ui.selected_lane', bridge)

    def test_physical_renderer_consumes_pattern_window(self):
        patch = (ROOT / 'tools' / 'apply_rb240_pattern_readback.py').read_text(encoding='utf-8')
        overlay = (ROOT / 'tools' / 'patch_fm1_rebirth_ui.py').read_text(encoding='utf-8')
        self.assertIn('refm_felucca_pattern_step', patch)
        self.assertIn('OFF/HIT/ACCENT', patch)
        self.assertIn('flags & 8u', patch)
        self.assertIn('flags & 2u', patch)
        self.assertIn('flags & 4u', patch)
        self.assertIn('apply_rb240_pattern_readback.py', overlay)
        self.assertIn('exact-pattern-readback', overlay)


if __name__ == '__main__':
    unittest.main()
