import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class WebSkinContractTests(unittest.TestCase):
    def test_rebirth_skin_assets_exist(self):
        css = ROOT / "web" / "emu" / "rebirth_skin.css"
        js = ROOT / "web" / "emu" / "rebirth_skin.js"
        self.assertTrue(css.is_file())
        self.assertTrue(js.is_file())
        self.assertIn("rebirth", css.read_text(encoding="utf-8").lower())

    def test_stock_303_and_mod_extension_are_separated(self):
        skin = (ROOT / "web" / "emu" / "rebirth_skin.js").read_text(encoding="utf-8")
        self.assertIn("STOCK 303", skin)
        self.assertIn("EXT / MOD", skin)
        self.assertIn("_refm_wasm_set_acid_tune", skin)

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
