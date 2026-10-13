#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Post-process the physical RB-240 renderer for real-LCD usability.

The first hardware validation showed two renderer problems that are not visible
in a desktop preview:

* the persistent 28 px help footer wastes scarce 240x240 space; and
* every parameter change clears all 57,600 pixels before repainting the page,
  producing an obvious flash on the FM-1 LCD.

This pass removes the persistent footer and limits full-screen clears to actual
page/machine transitions. Same-page value/step updates repaint their own panels
without blanking the display first.
"""
from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"RB-240 physical cleanup anchor mismatch in {path}: expected 1, got {count}: {old[:100]!r}"
        )
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("felucca", type=Path)
    args = parser.parse_args()
    root = args.felucca.resolve()
    draw = root / "firmware" / "src" / "ui_draw.c"

    footer_old = '''static void refm_footer(const char *text)\n{\n    cv_begin(240, 28, RB240_PANEL2);\n    cv_rect(0, 0, 240, 2, RB240_LIGHT);\n    cv_rect(4, 5, 232, 17, RB240_DARK);\n    cv_text(7, 8, &AF_S, text, RB240_TEXT);\n    cv_blit(0, 212);\n}\n'''
    footer_new = '''static void refm_footer(const char *text)\n{\n    /* Physical FM-1: persistent help text is deliberately omitted.\n       The reclaimed 28 px belongs to musical state, not instructions. */\n    (void)text;\n}\n'''
    replace_once(draw, footer_old, footer_new)

    redraw_old = '''    static uint32_t last_sig = 0xffffffffu;\n    char b[12];\n'''
    redraw_new = '''    static uint32_t last_sig = 0xffffffffu;\n    static uint16_t last_page = 0xffffu, last_machine = 0xffffu;\n    char b[12];\n'''
    replace_once(draw, redraw_old, redraw_new)

    clear_old = '''    if (sig == last_sig) return;\n    last_sig = sig;\n\n    lcd_fill(0, 0, 240, 240, RB240_BG);\n'''
    clear_new = '''    if (sig == last_sig) return;\n    last_sig = sig;\n\n    /* A full LCD clear is expensive and visibly flashes on real hardware.\n       Only page/machine transitions need it; same-page widgets paint their\n       own complete backing rectangles. */\n    if (page != last_page || machine != last_machine) {\n        lcd_fill(0, 0, 240, 240, RB240_BG);\n        last_page = page;\n        last_machine = machine;\n    }\n'''
    replace_once(draw, clear_old, clear_new)

    (root / ".refm-rb240-physical-cleanup").write_text(
        "footer=persistent-help-removed\n"
        "redraw=full-clear-only-on-page-or-machine-change\n"
        "logical-display=240x240\n"
    )
    print("RB-240 physical LCD cleanup applied: footer removed, same-page full clears eliminated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
