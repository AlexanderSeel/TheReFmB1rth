import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import check_target_budget as tb


SYMS = """
01c18000 g       .bss 00000000 _bss_end
01c20000 g       .pool 00000000 _pool_start
01c40000 g       .pool 00000000 _pool_end
"""


class TargetBudgetTests(unittest.TestCase):
    def test_inspect_regions(self):
        r = tb.inspect(400000, SYMS)
        self.assertEqual(r["ram"]["used"], 0x10000)
        self.assertEqual(r["pool"]["used"], 0x20000)
        self.assertEqual(r["xip"]["used"], 400000)

    def test_good_headroom_passes(self):
        r = tb.inspect(400000, SYMS)
        self.assertEqual(tb.violations(r, 1000, 1000, 1000), [])

    def test_ram_headroom_gate_fails(self):
        r = tb.inspect(400000, SYMS)
        errors = tb.violations(r, 1000, 40000, 1000)
        self.assertTrue(any("RAM headroom" in e for e in errors))

    def test_missing_symbol_rejected(self):
        with self.assertRaises(ValueError):
            tb.inspect(100, "01c18000 g .bss 0 _bss_end\n")


if __name__ == "__main__":
    unittest.main()
