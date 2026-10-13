#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Keep the FM-1 physical master gain entirely in 32-bit target arithmetic.

The JieLi pi32v2 compiler lowers the unnecessary int64_t multiply used by the
first hardware-rework pass to an undocumented ROM helper. The FM-1 ROM safety
gate correctly rejects that reference. Q15 sample * Q12 master and the second
Q15 headroom multiply each fit safely in signed 32-bit when performed as two
separate shifted operations.
"""
from __future__ import annotations

import argparse
from pathlib import Path

STAMP = ".refm-hardware-gain32"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-rework").is_file():
        raise SystemExit("apply hardware rework before 32-bit gain correction")

    audio = root / "firmware" / "src" / "audio.c"
    text = audio.read_text()
    old = """        int64_t v = ((int64_t)out[i] * master) >> 12;\n        v = (v * REFM_HW_HEADROOM_Q15) >> 15;\n        if (v < -32768) v = -32768;\n        if (v > 32767) v = 32767;\n        out[i] = (int32_t)v;\n"""
    new = """        int32_t v = (out[i] * master) >> 12;\n        v = (v * REFM_HW_HEADROOM_Q15) >> 15;\n        if (v < -32768) v = -32768;\n        if (v > 32767) v = 32767;\n        out[i] = v;\n"""
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"physical gain int64 anchor mismatch: expected 1, got {count}")
    audio.write_text(text.replace(old, new, 1))
    (root / STAMP).write_text(
        "gain-arithmetic=two-stage-signed-32bit\n"
        "rom-helper=none\n"
        "bounds=q15*q12-then-q15\n"
    )
    print("FM-1 physical master gain converted to target-safe 32-bit arithmetic")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
