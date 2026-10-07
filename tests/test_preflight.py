import hashlib
import importlib.util
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parents[1] / "tools" / "preflight.py"
spec = importlib.util.spec_from_file_location("preflight", MODULE_PATH)
preflight = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(preflight)


def make_package(product: str) -> bytes:
    raw = bytearray([0x7D] * (preflight.BLOCKS * preflight.BLK + 128))
    for i, ch in enumerate(product[: preflight.BLOCKS]):
        raw[i * preflight.BLK + preflight.KEEP] = (ord(ch) + i + 1) & 0xFF
    return bytes(raw)


class PreflightTests(unittest.TestCase):
    def test_product_of_roundtrip(self):
        self.assertEqual(preflight.product_of(make_package("FM-1_910")), "FM-1_910")

    def test_product_rejects_short_file(self):
        with self.assertRaisesRegex(ValueError, "too short"):
            preflight.product_of(b"too short")

    def test_sha256(self):
        raw = b"TheReFmB1rth"
        self.assertEqual(preflight.sha256(raw), hashlib.sha256(raw).hexdigest())


if __name__ == "__main__":
    unittest.main()
