#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Keep the FM-1 physical target free of unsupported arithmetic helpers.

The JieLi pi32v2 compiler lowers some otherwise ordinary C operations to helper
symbols that are not provided by this FM-1 link environment. The physical
master gain therefore stays in signed 32-bit fixed-point arithmetic, and the
x0x-style 303 target copy avoids direct uint32_t<->float conversions while
retaining the exact same audible float DSP state.
"""
from __future__ import annotations

import argparse
from pathlib import Path

STAMP = ".refm-hardware-gain32"


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label} anchor mismatch: expected 1, got {count}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-rework").is_file():
        raise SystemExit("apply hardware rework before target arithmetic correction")

    audio = root / "firmware" / "src" / "audio.c"
    old = """        int64_t v = ((int64_t)out[i] * master) >> 12;\n        v = (v * REFM_HW_HEADROOM_Q15) >> 15;\n        if (v < -32768) v = -32768;\n        if (v > 32767) v = 32767;\n        out[i] = (int32_t)v;\n"""
    new = """        int32_t v = (out[i] * master) >> 12;\n        v = (v * REFM_HW_HEADROOM_Q15) >> 15;\n        if (v < -32768) v = -32768;\n        if (v > 32767) v = 32767;\n        out[i] = v;\n"""
    replace_once(audio, old, new, "physical gain int64")

    acid = root / "firmware" / "refm" / "proto" / "acid303.c"
    replace_once(
        acid,
        """static float inc_to_hz(uint32_t inc) {\n    return (float)inc * (SR / 4294967296.0f);\n}\n""",
        """static float inc_to_hz(uint32_t inc) {\n    uint32_t hi = inc >> 16;\n    uint32_t lo = inc & 65535u;\n    float f = (float)(int32_t)hi * 65536.0f + (float)(int32_t)lo;\n    return f * (SR / 4294967296.0f);\n}\n""",
        "303 uint32-to-float",
    )
    replace_once(
        acid,
        """    s->phase = (uint32_t)(s->f_phase * 4294967040.0f);\n""",
        """    {\n        int32_t phase_signed = (int32_t)(s->f_phase * 2147483520.0f);\n        s->phase = ((uint32_t)phase_signed) << 1;\n    }\n""",
        "303 float-to-uint32 diagnostic mirror",
    )

    (root / STAMP).write_text(
        "gain-arithmetic=two-stage-signed-32bit\n"
        "303-u32-float=split-16bit-signed-conversion\n"
        "303-phase-mirror=signed-conversion-plus-shift\n"
        "rom-helper=none\n"
        "bounds=q15*q12-then-q15\n"
    )
    print("FM-1 physical master gain and 303 conversions made target-safe")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
