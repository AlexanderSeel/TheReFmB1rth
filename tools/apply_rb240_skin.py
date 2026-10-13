#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the RB-240 visual skin to the generated FM-1 hardware renderer.

This is deliberately a renderer-only pass. It does not touch ui240 state,
transport, keyboard scanning, MIDI, audio, updater, or persistence semantics.
The same palette/geometry is mirrored by web/app/virtual_fm1.js.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"RB-240 skin anchor mismatch in {path}: {old[:80]!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    draw = root / "firmware" / "src" / "ui_draw.c"

    replace_once(
        draw,
        "static uint32_t refm_pct(uint32_t v) { return (v * 100u + 16383u) / 32767u; }\n",
        r'''/* RB-240: compact metallic groovebox skin, RGB565. */
#define RB240_BG       0x2945u
#define RB240_PANEL    0x6B4Du
#define RB240_PANEL2   0x528Au
#define RB240_DARK     0x2104u
#define RB240_LIGHT    0xB596u
#define RB240_TEXT     0xFFFFu
#define RB240_DIM      0xC638u
#define RB240_LED_BG   0x1800u
#define RB240_RED      0xF986u
#define RB240_AMBER    0xFD20u
#define RB240_CYAN     0x2D7Fu
#define RB240_GREEN    0x4FE8u

static uint32_t refm_pct(uint32_t v) { return (v * 100u + 16383u) / 32767u; }

static void refm_bevel(int32_t w, int32_t h)
{
    cv_rect(0, 0, w, 2, RB240_LIGHT);
    cv_rect(0, 0, 2, h, RB240_LIGHT);
    cv_rect(0, h - 2, w, 2, RB240_DARK);
    cv_rect(w - 2, 0, 2, h, RB240_DARK);
}
''',
    )

    replace_once(
        draw,
        '''static void refm_card_val(int32_t x, int32_t y, const char *label, int32_t value)\n{\n    char b[12];\n    cv_begin(56, 44, T_SURF);\n    cv_text(4, 3, &AF_S, label, T_MID);\n    fmt_int(b, value);\n    cv_text(4, 21, &AF_M, b, T_THEME);\n    cv_blit(x, y);\n}\n''',
        '''static void refm_card_val(int32_t x, int32_t y, const char *label, int32_t value)\n{\n    char b[12];\n    cv_begin(56, 44, RB240_PANEL);\n    refm_bevel(56, 44);\n    cv_text(5, 3, &AF_S, label, RB240_TEXT);\n    cv_rect(5, 18, 46, 20, RB240_LED_BG);\n    fmt_int(b, value);\n    cv_text(8, 21, &AF_M, b, RB240_RED);\n    cv_blit(x, y);\n}\n''',
    )

    replace_once(
        draw,
        '''static void refm_card_text(int32_t x, int32_t y, const char *label, const char *value)\n{\n    cv_begin(56, 44, T_SURF);\n    cv_text(4, 3, &AF_S, label, T_MID);\n    cv_text(4, 21, &AF_M, value, T_THEME);\n    cv_blit(x, y);\n}\n''',
        '''static void refm_card_text(int32_t x, int32_t y, const char *label, const char *value)\n{\n    cv_begin(56, 44, RB240_PANEL);\n    refm_bevel(56, 44);\n    cv_text(5, 3, &AF_S, label, RB240_TEXT);\n    cv_rect(5, 18, 46, 20, RB240_LED_BG);\n    cv_text(8, 21, &AF_M, value, RB240_AMBER);\n    cv_blit(x, y);\n}\n''',
    )

    replace_once(
        draw,
        '''static void refm_footer(const char *text)\n{\n    cv_begin(240, 28, T_BG);\n    cv_text(5, 7, &AF_S, text, T_MID);\n    cv_blit(0, 212);\n}\n''',
        '''static void refm_footer(const char *text)\n{\n    cv_begin(240, 28, RB240_PANEL2);\n    cv_rect(0, 0, 240, 2, RB240_LIGHT);\n    cv_text(5, 7, &AF_S, text, RB240_TEXT);\n    cv_blit(0, 212);\n}\n''',
    )

    replace_once(
        draw,
        '''        uint16_t c = i == selected ? T_ACCENT : i == playing ? T_THEME : T_SURF;\n        lcd_fill(x, y, 22, 18, c);\n''',
        '''        uint16_t c = i == selected ? RB240_AMBER : i == playing ? RB240_GREEN : RB240_DARK;\n        lcd_fill(x, y, 22, 18, RB240_PANEL);\n        lcd_fill(x + 3, y + 3, 16, 12, c);\n''',
    )

    replace_once(
        draw,
        '''    lcd_fill(0, 0, 240, 240, T_BG);\n    cv_begin(240, 24, T_BG);\n    cv_text(5, HEAD_MY, &AF_M, pn, T_THEME);\n    cv_text(76, HEAD_MY, &AF_M, mn, T_TEXT);\n    fmt_int(b, bpm);\n    cv_text(177, HEAD_MY, &AF_M, b, T_THEME);\n    cv_text(213, HEAD_SY, &AF_S, playing ? "PLAY" : "STOP", playing ? T_ACCENT : T_MID);\n    cv_blit(0, 0);\n\n    cv_begin(240, 22, T_BG);\n    cv_text(5, 5, &AF_S, "PAT", T_MID);\n    b[0] = (char)('A' + (pattern & 7u)); b[1] = 0;\n    cv_text(34, 5, &AF_S, b, T_THEME);\n    cv_text(72, 5, &AF_S, "PRESET=PAT  ALGO=TRACK  SELECT=BPM", T_MID);\n    cv_blit(0, 27);\n''',
        '''    lcd_fill(0, 0, 240, 240, RB240_BG);\n    cv_begin(240, 24, RB240_PANEL2);\n    cv_rect(0, 0, 240, 2, RB240_LIGHT);\n    cv_rect(0, 22, 240, 2, RB240_DARK);\n    cv_text(5, HEAD_MY, &AF_M, pn, RB240_AMBER);\n    cv_text(76, HEAD_MY, &AF_M, mn, RB240_TEXT);\n    cv_rect(171, 4, 36, 16, RB240_LED_BG);\n    fmt_int(b, bpm);\n    cv_text(177, HEAD_MY, &AF_M, b, RB240_RED);\n    cv_text(213, HEAD_SY, &AF_S, playing ? "PLAY" : "STOP", playing ? RB240_GREEN : RB240_DIM);\n    cv_blit(0, 0);\n\n    cv_begin(240, 22, RB240_PANEL);\n    cv_rect(0, 0, 240, 2, RB240_LIGHT);\n    cv_rect(0, 20, 240, 2, RB240_DARK);\n    cv_text(5, 5, &AF_S, "PATTERN", RB240_TEXT);\n    b[0] = (char)('A' + (pattern & 7u)); b[1] = 0;\n    cv_rect(47, 3, 17, 15, RB240_LED_BG);\n    cv_text(52, 5, &AF_S, b, RB240_RED);\n    cv_text(72, 5, &AF_S, "PRESET  ALGO  SELECT", RB240_DIM);\n    cv_blit(0, 27);\n''',
    )

    marker = root / ".refm-rb240-skin"
    marker.write_text("style=rb240-metal-panel-v1\nvirtual-contract=web/app/virtual_fm1.js\n")
    print("RB-240 metallic hardware LCD skin applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
