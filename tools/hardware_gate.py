#!/usr/bin/env python3
"""Safety gate for experimental FM-1 firmware installation.

This module never writes firmware. It validates the package identity/hash and
requires explicit acknowledgement before a separate installer may be invoked.
"""
from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import sys

BLOCKS = 20
BLK = 0x30
KEEP = 0x2F
CONFIRM_TEXT = "FLASH THE FM-1"
FM1_ID_RE = re.compile(r"^(?:ota-)?FM-1_\d+$", re.I)


def product_of(raw: bytes) -> str:
    if len(raw) < BLOCKS * BLK:
        raise ValueError("package is too short")
    chars: list[str] = []
    for i in range(BLOCKS):
        marker = raw[i * BLK + KEEP]
        if marker != 0x7D:
            chars.append(chr((marker - i - 1) & 0xFF))
    product = "".join(chars)
    if not FM1_ID_RE.fullmatch(product):
        raise ValueError(f"unexpected FM-1 product identity: {product!r}")
    return product


def digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def validate_gate(raw: bytes, expected_sha256: str, device_identity: str, experimental: bool, confirmation: str) -> list[str]:
    errors: list[str] = []
    try:
        product = product_of(raw)
    except ValueError as exc:
        return [str(exc)]
    actual = digest(raw)
    if actual.lower() != expected_sha256.strip().lower():
        errors.append("SHA-256 mismatch")
    if not FM1_ID_RE.fullmatch(device_identity):
        errors.append("connected device identity is not an FM-1/OTA FM-1 identity")
    else:
        package_model = product.removeprefix("ota-").split("_", 1)[0]
        device_model = device_identity.removeprefix("ota-").split("_", 1)[0]
        if package_model.upper() != device_model.upper():
            errors.append("package and device model differ")
    if not experimental:
        errors.append("--experimental is required")
    if confirmation != CONFIRM_TEXT:
        errors.append(f"confirmation must be exactly {CONFIRM_TEXT!r}")
    return errors


def main() -> int:
    p = argparse.ArgumentParser(description="Non-writing safety gate for experimental FM-1 firmware")
    p.add_argument("package", type=pathlib.Path)
    p.add_argument("--sha256", required=True, help="expected SHA-256 from a trusted release manifest")
    p.add_argument("--device-identity", required=True, help="identity already read from the connected FM-1")
    p.add_argument("--experimental", action="store_true")
    p.add_argument("--confirm", default="")
    args = p.parse_args()
    try:
        raw = args.package.read_bytes()
    except OSError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    errors = validate_gate(raw, args.sha256, args.device_identity, args.experimental, args.confirm)
    if errors:
        for error in errors:
            print(f"BLOCKED: {error}", file=sys.stderr)
        print("No firmware was written.")
        return 6
    print(f"Gate passed for {product_of(raw)}; SHA256 {digest(raw)}")
    print("This command is non-writing. A loader-mediated installer is still required.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
