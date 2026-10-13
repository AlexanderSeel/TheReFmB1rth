#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Add an eight-position FOCUS location indicator to the physical RB-240 renderer."""
from __future__ import annotations
import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    draw = root / "firmware" / "src" / "ui_draw.c"
    text = draw.read_text()
    anchor = '    refm_led_dot(126, 42, 1u, RB240_RED); refm_led_dot(165, 42, 0u, RB240_RED); refm_led_dot(211, 42, 0u, RB240_RED);\n'
    indicator = (
        '    if (refm_felucca_ui_value(34u) != 0u) {\n'
        '        uint8_t focus_region = (uint8_t)(refm_felucca_ui_value(35u) & 7u);\n'
        '        for (uint8_t i = 0u; i < 8u; ++i) refm_led_dot((int)(48u + i * 6u), 7, i == focus_region, RB240_CYAN);\n'
        '    }\n'
    )
    if text.count(anchor) != 1:
        raise SystemExit("RB-240 focus indicator anchor mismatch")
    draw.write_text(text.replace(anchor, anchor + indicator, 1))
    (root / ".refm-rb240-focus").write_text(
        "focus-indicator=eight-position-cyan-header-map\n"
        "active-state=ui-value-34\n"
        "region-state=ui-value-35\n"
    )
    print("RB-240 hardware FOCUS location indicator applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
