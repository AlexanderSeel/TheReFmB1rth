#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Remove target-time divisions from the injected ReFm beta LCD renderer.

The JieLi compiler may lower integer division to undocumented ROM helpers. Felucca
intentionally rejects those references, so the display uses Q15/Q7 approximations
instead. These are UI percentages only and do not affect audio/control values.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"target beta ROM-fix anchor mismatch in {path}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    draw = args.felucca.resolve() / "firmware" / "src" / "ui_draw.c"

    replace_once(
        draw,
        "static uint32_t refm_pct(uint32_t v) { return (v * 100u + 16383u) / 32767u; }",
        "static uint32_t refm_pct(uint32_t v) { return (v * 100u + 16384u) >> 15; }",
    )
    replace_once(
        draw,
        "((int32_t)p1 - 32767) * 100 / 32767",
        "(((int32_t)p1 - 32767) * 100) >> 15",
    )
    replace_once(
        draw,
        "(int32_t)p2 * 100 / 127",
        "((int32_t)p2 * 100) >> 7",
    )
    replace_once(
        draw,
        "(int32_t)p3 * 100 / 127",
        "((int32_t)p3 * 100) >> 7",
    )
    replace_once(
        draw,
        "(int32_t)p1 * 100 / 31000",
        "((int32_t)p1 * 100) >> 15",
    )
    print("FM-1 beta renderer ROM-helper divisions removed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
