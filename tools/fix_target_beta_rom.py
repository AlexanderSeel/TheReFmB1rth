#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Remove target-time divisions and ROM-range immediates from the ReFm beta LCD renderer.

Felucca rejects any instruction that materializes a value in the JieLi chip-ROM
address window. That includes innocent signed arithmetic constants such as
-32767*100 == 0xffce0064. Keep all display-only scaling in small Q7/Q15 steps so
those values are never emitted. The beta3 modular FX renderer consumes values
already normalized by ui240, so its old delay-feedback display expression no
longer exists and is intentionally optional here.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"target beta ROM-fix anchor mismatch in {path}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def replace_optional(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    count = text.count(old)
    if count > 1:
        raise SystemExit(f"target beta ROM-fix optional anchor repeated in {path}: {old!r}")
    if count == 1:
        path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    draw = args.felucca.resolve() / "firmware" / "src" / "ui_draw.c"

    replace_once(draw,
        "static uint32_t refm_pct(uint32_t v) { return (v * 100u + 16383u) / 32767u; }",
        "static uint32_t refm_pct(uint32_t v) { return (v * 100u + 16384u) >> 15; }")
    replace_once(draw,
        "((int32_t)p1 - 32767) * 100 / 32767",
        "((((int32_t)p1 - 32767) >> 8) * 100) >> 7")
    replace_once(draw,
        "(int32_t)p2 * 100 / 127",
        "((int32_t)p2 * 100) >> 7")
    replace_once(draw,
        "(int32_t)p3 * 100 / 127",
        "((int32_t)p3 * 100) >> 7")
    replace_optional(draw,
        "(int32_t)p1 * 100 / 31000",
        "((int32_t)p1 * 100) >> 15")
    print("FM-1 beta renderer target-safe display arithmetic applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
