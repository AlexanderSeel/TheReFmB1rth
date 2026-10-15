#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Give the physical FM-1 303 square a restrained organ-like body.

The core remains the antialiased polyBLEP square. A small triangle component is
mixed before the ladder so the square retains its 303 edge but has a rounder
fundamental/low-mid body on hardware. This is target-only until the physical
voicing is approved; the shared DSP API/project format is unchanged.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    path = args.felucca.resolve() / "firmware" / "refm" / "proto" / "acid303.c"
    text = path.read_text()
    old = '''        y -= blep(t, inc);\n        y *= -0.5f;\n    }\n    p += inc;\n'''
    new = '''        y -= blep(t, inc);\n        y *= -0.5f;\n        /* 82% polyBLEP square + 18% triangle: still recognisably 303 square,\n         * but with a warmer organ-like fundamental and less brittle top end. */\n        {\n            float tri = p < 0.5f ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);\n            y = 0.82f * y + 0.18f * tri;\n        }\n    }\n    p += inc;\n'''
    if text.count(old) != 1:
        raise SystemExit("square oscillator anchor changed")
    path.write_text(text.replace(old, new, 1))
    print("FM-1 square oscillator warmed with restrained triangle body")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
