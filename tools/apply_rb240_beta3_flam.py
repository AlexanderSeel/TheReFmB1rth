#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bind the approved 909 FLAM controls to the real beta3 flam sequencer state."""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"RB-240 flam anchor mismatch in {path}: {old[:100]!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    draw = args.felucca.resolve() / "firmware" / "src" / "ui_draw.c"

    # p2 = selected 909 step/lane flam flag, p3 = global flam width percentage.
    replace_once(
        draw,
        '        refm_led_dot(198, 103, 0u, RB240_RED); refm_led_dot(218, 103, playing != 0u, RB240_RED);\n',
        '        refm_led_dot(198, 103, machine == 3u && p2 != 0u, RB240_AMBER); refm_led_dot(218, 103, playing != 0u, RB240_RED);\n',
    )
    replace_once(
        draw,
        '        for (i = 0; i < 6u; ++i) refm_knob(11 + (int32_t)i * 29, 174, 0u, RB240_AMBER);\n',
        '        for (i = 0; i < 5u; ++i) refm_knob(11 + (int32_t)i * 29, 174, 0u, RB240_AMBER);\n'
        '        refm_knob(156, 174, machine == 3u ? p3 : 0u, RB240_AMBER);\n',
    )
    replace_once(
        draw,
        '        refm_footer(machine == 2u ? "K1 LANE K2 OFF/HIT/ACC | PRESET STEP" : "K1 LANE K2 OFF/LOW/HIGH | FLAM UI BETA");\n',
        '        refm_footer(machine == 2u ? "K1 LANE K2 OFF/HIT/ACC | PRESET STEP" : "K1 LANE K2 LOW/HIGH K3 FLAM K4 WIDTH");\n',
    )
    marker = args.felucca.resolve() / ".refm-rb240-beta3-flam"
    marker.write_text("909-flam=real\ncontrols=K3-toggle,K4-width\n")
    print("RB-240 real 909 flam UI binding applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
