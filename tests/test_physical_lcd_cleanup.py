# SPDX-License-Identifier: GPL-3.0-only
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class PhysicalLcdCleanupTests(unittest.TestCase):
    def test_cleanup_removes_persistent_footer_and_full_clear_flash(self):
        s = (ROOT / 'tools' / 'apply_rb240_physical_cleanup.py').read_text(encoding='utf-8')
        self.assertIn('(void)text;', s)
        self.assertIn('last_page = 0xffffu', s)
        self.assertIn('last_machine = 0xffffu', s)
        self.assertIn('if (page != last_page || machine != last_machine)', s)
        self.assertIn('full-clear-only-on-page-or-machine-change', s)
        self.assertNotIn('cv_blit(0, 212);\n}\n\'\'\'\n    footer_new', s)

    def test_cleanup_is_part_of_physical_overlay(self):
        s = (ROOT / 'tools' / 'patch_fm1_rebirth_ui.py').read_text(encoding='utf-8')
        self.assertIn('apply_rb240_physical_cleanup.py', s)
        self.assertIn('physical-lcd-cleanup', s)


if __name__ == '__main__':
    unittest.main()
