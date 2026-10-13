#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render exact 16-step groovebox state on the physical RB-240 LCD.

Virtual FM-1 already receives a 16-step pattern window. The hardware renderer
must not reconstruct a sequence from selected_step/play_step either, so this
post-pass binds its ACID and DRUM grids to refm_felucca_pattern_step().

For ACID, each populated step also renders the actual musical note name. This
fixes the hardware usability problem where a user could not tell which pitch
was stored in a step even when the underlying gate/note data changed.
"""
from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"RB-240 pattern-readback anchor mismatch in {path}: expected 1, got {count}: {old[:120]!r}"
        )
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("felucca", type=Path)
    args = parser.parse_args()
    root = args.felucca.resolve()
    draw = root / "firmware" / "src" / "ui_draw.c"

    replace_once(
        draw,
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\n",
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\n"
        "extern uint32_t refm_felucca_pattern_step(uint8_t step);\n",
    )

    drum_old = '''static void refm_steps(uint16_t selected, uint16_t playing)\n{\n    uint32_t i;\n    for (i = 0; i < 16u; ++i) {\n        int32_t x = 8 + (int32_t)(i & 7u) * 29;\n        int32_t y = 77 + (int32_t)(i >> 3) * 27;\n        uint16_t c = i == selected ? RB240_AMBER : i == playing ? RB240_GREEN : RB240_PANEL2;\n        lcd_fill(x, y, 22, 18, RB240_DARK);\n        lcd_fill(x + 2, y + 2, 18, 14, RB240_PANEL);\n        lcd_fill(x + 4, y + 4, 14, 10, c);\n    }\n}\n'''
    drum_new = '''static void refm_note_name(uint8_t note, char out[5])\n{\n    static const char *const nn[12] = {\"C\",\"C#\",\"D\",\"D#\",\"E\",\"F\",\"F#\",\"G\",\"G#\",\"A\",\"A#\",\"B\"};\n    const char *n = nn[note % 12u];\n    int32_t oct = (int32_t)(note / 12u) - 1;\n    uint8_t p = 0u;\n    out[p++] = n[0];\n    if (n[1]) out[p++] = n[1];\n    if (oct < 0) out[p++] = '-';\n    else out[p++] = (char)('0' + (oct > 9 ? 9 : oct));\n    out[p] = 0;\n}\n\nstatic void refm_steps(uint16_t selected, uint16_t playing)\n{\n    uint32_t i;\n    for (i = 0; i < 16u; ++i) {\n        uint32_t packed = refm_felucca_pattern_step((uint8_t)i);\n        uint8_t state = (uint8_t)(packed & 3u);\n        uint8_t flam = (uint8_t)((packed & 4u) != 0u);\n        int32_t x = 8 + (int32_t)(i & 7u) * 29;\n        int32_t y = 77 + (int32_t)(i >> 3) * 27;\n        uint16_t c = state == 2u ? RB240_RED : state == 1u ? RB240_AMBER : RB240_PANEL2;\n        lcd_fill(x, y, 22, 18, RB240_DARK);\n        lcd_fill(x + 2, y + 2, 18, 14, RB240_PANEL);\n        lcd_fill(x + 4, y + 4, 14, 10, c);\n        if (flam) lcd_fill(x + 16, y + 3, 3, 3, RB240_CYAN);\n        if (i == selected) {\n            lcd_fill(x, y, 22, 1, RB240_TEXT);\n            lcd_fill(x, y + 17, 22, 1, RB240_TEXT);\n        }\n        if (i == playing) lcd_fill(x + 2, y + 15, 18, 2, RB240_GREEN);\n    }\n}\n'''
    replace_once(draw, drum_old, drum_new)

    acid_old = '''        for (i = 0; i < 16u; ++i) {\n            int32_t sx = 5 + (int32_t)i * 14;\n            refm_button(sx, 178, 12, 18, "", i == sel_step || (playing && i == play_step), i == sel_step ? RB240_RED : RB240_GREEN);\n        }\n'''
    acid_new = '''        for (i = 0; i < 16u; ++i) {\n            uint32_t packed = refm_felucca_pattern_step((uint8_t)i);\n            uint8_t note = (uint8_t)(packed & 0xffu);\n            uint8_t flags = (uint8_t)((packed >> 8) & 0xffu);\n            uint8_t gate = (uint8_t)((flags & 1u) != 0u);\n            uint8_t accent_step = (uint8_t)((flags & 2u) != 0u);\n            uint8_t slide_step = (uint8_t)((flags & 4u) != 0u);\n            uint8_t tie_step = (uint8_t)((flags & 8u) != 0u && !gate);\n            int32_t sx = 8 + (int32_t)(i & 7u) * 29;\n            int32_t sy = 174 + (int32_t)(i >> 3) * 31;\n            uint16_t sc = tie_step ? RB240_CYAN : accent_step && gate ? RB240_RED : gate ? RB240_AMBER : RB240_PANEL2;\n            char nl[5];\n            lcd_fill(sx, sy, 22, 25, RB240_DARK);\n            lcd_fill(sx + 2, sy + 2, 18, 21, RB240_PANEL);\n            if (gate || tie_step) lcd_fill(sx + 3, sy + 3, 16, 17, sc);\n            if (gate) refm_note_name(note, nl);\n            else if (tie_step) { nl[0] = 'T'; nl[1] = 'I'; nl[2] = 'E'; nl[3] = 0; }\n            else { nl[0] = '-'; nl[1] = '-'; nl[2] = 0; }\n            cv_begin(18, 12, gate || tie_step ? sc : RB240_PANEL);\n            cv_text(1, 2, &AF_S, nl, gate || tie_step ? RB240_BLACK : RB240_MID);\n            cv_blit(sx + 2, sy + 5);\n            if (slide_step) lcd_fill(sx + 3, sy + 20, 16, 2, RB240_CYAN);\n            if (i == sel_step) {\n                lcd_fill(sx, sy, 22, 2, RB240_TEXT);\n                lcd_fill(sx, sy + 23, 22, 2, RB240_TEXT);\n            }\n            if (playing && i == play_step) lcd_fill(sx + 20, sy + 2, 2, 21, RB240_GREEN);\n        }\n'''
    replace_once(draw, acid_old, acid_new)

    (root / ".refm-rb240-pattern-readback").write_text(
        "acid=exact-16-step-note-name-flags-probability-source\n"
        "acid-layout=8x2-visible-note-grid\n"
        "drum=exact-selected-lane-off-hit-accent-flam\n"
        "source=refm_felucca_pattern_step\n"
    )
    print("RB-240 exact ACID note-name/drum 16-step pattern readback applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
