#!/usr/bin/env python3
"""Non-destructive preflight checks for FM-1 firmware packages.

This tool deliberately performs no flash writes. It validates basic package
structure, extracts the embedded product marker using the Felucca package
layout, prints SHA-256, and can optionally list likely FM-1 MIDI ports.

It is a bootstrap safety tool; the final installer must use the proven Felucca
update protocol and stronger package validation/tests.
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
PORT_RE = re.compile(r"fm-1|felucca|ota|composite|sinco|usb-midi", re.I)


def product_of(raw: bytes) -> str:
    if len(raw) < BLOCKS * BLK:
        raise ValueError("file is too short to be an FM-1 .fwsc package")
    chars: list[str] = []
    for i in range(BLOCKS):
        marker = raw[i * BLK + KEEP]
        if marker != 0x7D:
            chars.append(chr((marker - i - 1) & 0xFF))
    product = "".join(chars)
    if not product or "_" not in product:
        raise ValueError(f"could not extract a plausible FM-1 product identity: {product!r}")
    return product


def sha256(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def list_ports() -> int:
    try:
        import mido  # type: ignore
    except ImportError:
        print("MIDI check skipped: install mido + python-rtmidi to enumerate ports.")
        return 0

    inputs = list(dict.fromkeys(mido.get_input_names()))
    outputs = list(dict.fromkeys(mido.get_output_names()))
    likely_in = [name for name in inputs if PORT_RE.search(name)]
    likely_out = [name for name in outputs if PORT_RE.search(name)]

    print("Likely FM-1 MIDI inputs:")
    for name in likely_in:
        print(f"  - {name}")
    print("Likely FM-1 MIDI outputs:")
    for name in likely_out:
        print(f"  - {name}")

    if not likely_in or not likely_out:
        print("WARNING: no complete likely FM-1 MIDI pair found.")
        return 3
    if len(likely_in) > 1 or len(likely_out) > 1:
        print("WARNING: multiple candidate ports found; a future installer must require explicit selection.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Read-only FM-1 firmware preflight")
    parser.add_argument("package", nargs="?", type=pathlib.Path, help="FM-1 .fwsc package to inspect")
    parser.add_argument("--ports", action="store_true", help="list likely FM-1 MIDI ports (no messages sent)")
    parser.add_argument("--expect-product", help="require exact embedded product identity")
    args = parser.parse_args()

    status = 0
    if args.package:
        try:
            raw = args.package.read_bytes()
            product = product_of(raw)
        except (OSError, ValueError) as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 2

        digest = sha256(raw)
        print(f"File:    {args.package}")
        print(f"Bytes:   {len(raw)}")
        print(f"Product: {product}")
        print(f"SHA256:  {digest}")

        if args.expect_product and product != args.expect_product:
            print(
                f"ERROR: expected product {args.expect_product!r}, package contains {product!r}",
                file=sys.stderr,
            )
            status = 6

    if args.ports:
        status = max(status, list_ports())

    if not args.package and not args.ports:
        parser.error("provide a package and/or --ports")

    print("Preflight is read-only: no firmware was written.")
    return status


if __name__ == "__main__":
    raise SystemExit(main())
