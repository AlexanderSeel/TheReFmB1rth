import hashlib
import importlib.util
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


def test_product_of_roundtrip():
    raw = make_package("FM-1_910")
    assert preflight.product_of(raw) == "FM-1_910"


def test_product_rejects_short_file():
    try:
        preflight.product_of(b"too short")
    except ValueError as exc:
        assert "too short" in str(exc)
    else:
        raise AssertionError("expected ValueError")


def test_sha256():
    raw = b"TheReFmB1rth"
    assert preflight.sha256(raw) == hashlib.sha256(raw).hexdigest()
