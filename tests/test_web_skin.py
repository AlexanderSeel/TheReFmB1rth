import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class WebSkinContractTests(unittest.TestCase):
    def test_hardware_rack_skin_assets_exist(self):
        css = ROOT / "web" / "emu" / "rebirth_skin.css"
        js = ROOT / "web" / "emu" / "rebirth_skin.js"
        self.assertTrue(css.is_file())
        self.assertTrue(js.is_file())
        style = css.read_text(encoding="utf-8").lower()
        self.assertIn("hardware-rack", style)
        self.assertIn("refm-drum-808", style)
        self.assertIn("refm-drum-909", style)
        self.assertIn("refm-mixer-rack", style)

    def test_stock_303_and_mod_extension_are_separated(self):
        skin = (ROOT / "web" / "emu" / "rebirth_skin.js").read_text(encoding="utf-8")
        self.assertIn("TB STYLE", skin)
        self.assertIn("SAW / SQUARE", skin)
        self.assertIn("EXT / MOD", skin)
        self.assertIn("STEP PROGRAMMER", skin)
        self.assertIn("ACCENT", skin)
        self.assertIn("SLIDE", skin)
        self.assertIn("TIE", skin)

    def test_dual_drum_rack_and_mixer_are_real_wasm_controls(self):
        skin = (ROOT / "web" / "emu" / "rebirth_skin.js").read_text(encoding="utf-8")
        build = (ROOT / "web" / "emu" / "build_wasm.sh").read_text(encoding="utf-8")
        samples = (ROOT / "web" / "emu" / "sample_controls.js").read_text(encoding="utf-8")
        self.assertIn("DRUM ${track?'909':'808'}", skin)
        self.assertIn("MASTER FX", skin)
        self.assertIn("_refm_wasm_set_mix_track", skin)
        self.assertIn("_refm_wasm_set_fx", skin)
        self.assertIn("_refm_wasm_set_sample_lane", samples)
        self.assertIn("_refm_wasm_sample_active_mask", samples)
        self.assertIn("_refm_wasm_set_mix_track", build)
        self.assertIn("_refm_wasm_set_sample_lane", build)

    def test_wasm_build_exports_tune_and_packages_skin(self):
        build = (ROOT / "web" / "emu" / "build_wasm.sh").read_text(encoding="utf-8")
        self.assertIn("_refm_wasm_set_acid_tune", build)
        self.assertIn("rebirth_skin.css", build)
        self.assertIn("rebirth_skin.js", build)

    def test_fm1_compact_skin_patch_exists(self):
        patch = (ROOT / "tools" / "patch_fm1_rebirth_ui.py").read_text(encoding="utf-8")
        self.assertIn("16 grouped step lamps", patch)
        self.assertIn("compact-rebirth-inspired", patch)


if __name__ == "__main__":
    unittest.main()
