#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Add the small FOCUS mode indicator to the generated physical RB-240 renderer."""
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
    replacement = anchor + '    if (refm_felucca_ui_value(34u) != 0u) refm_led_dot(56, 7, 1u, RB240_CYAN);\n'
    if text.count(anchor) != 1:
        raise SystemExit("RB-240 focus indicator anchor mismatch")
    draw.write_text(text.replace(anchor, replacement, 1))
    (root / ".refm-rb240-focus").write_text("focus-indicator=cyan-header-dot\nstate=ui-value-34\n")
    print("RB-240 hardware FOCUS indicator applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
