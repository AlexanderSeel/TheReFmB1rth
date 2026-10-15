import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class HardwareWasmFullParityTests(unittest.TestCase):
    def test_final_overlay_owns_full_wasm_hierarchy(self):
        patch = (ROOT / "tools" / "patch_fm1_rebirth_ui.py").read_text(encoding="utf-8")
        self.assertIn("apply_rb240_full_parity.py", patch)
        self.assertIn("full-wasm-rack-hierarchy", patch)

    def test_parity_pass_covers_every_user_page(self):
        patch = (ROOT / "tools" / "apply_rb240_full_parity.py").read_text(encoding="utf-8")
        for marker in (
            '"HOME"', '"ACID"', '"SEQ EDIT"', '"DRUM"', '"MIXER"',
            '"FX"', '"SONG"', '"PROJECT"',
            "11x16-full-matrix", "four-channels-simultaneous", "six-master-controls",
        ):
            self.assertIn(marker, patch)
        self.assertIn("refm_felucca_drum_matrix_value", patch)
        self.assertIn("refm_felucca_mix_value", patch)
        self.assertIn("refm_felucca_fx_value", patch)

    def test_factory_demo_does_not_get_overwritten_by_old_beta_seed(self):
        patch = (ROOT / "tools" / "apply_rb240_full_parity.py").read_text(encoding="utf-8")
        self.assertIn("groovebox_pattern_load(&g_refm.groovebox.patterns", patch)
        self.assertIn("136u", patch)

    def test_factory_bank_is_original_csharp_minor_rave_set(self):
        bank = (ROOT / "firmware" / "proto" / "pattern_bank.c").read_text(encoding="utf-8")
        self.assertIn("C# minor", bank)
        self.assertIn("dark 136 BPM blood-rave / acid", bank)
        self.assertIn("{37,37,44,37,40,37,47,44", bank)
        self.assertIn("HIT(0)|HIT(1)|HIT(4)", bank)


if __name__ == "__main__":
    unittest.main()
