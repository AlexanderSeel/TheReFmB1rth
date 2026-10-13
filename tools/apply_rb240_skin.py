#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the approved RB-240 hardware/virtual visual contract.

Renderer-only transformation. State, panel input, transport, MIDI, audio,
persistence and updater semantics remain owned by the existing beta bridge.
The coordinates in this file are mirrored by web/app/rb240_renderer.js.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"RB-240 skin anchor mismatch in {path}: {old[:100]!r}")
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
        r'''/* RB-240 v3: approved dense brushed-metal groovebox skin, RGB565. */
#define RB240_BG       0x18E3u
#define RB240_PANEL    0x9CF3u
#define RB240_PANEL2   0x632Cu
#define RB240_DARK     0x18E3u
#define RB240_BLACK    0x0841u
#define RB240_LIGHT    0xD69Au
#define RB240_TEXT     0xFFFFu
#define RB240_INK      0x1082u
#define RB240_DIM      0xBDF7u
#define RB240_LED_BG   0x1800u
#define RB240_RED      0xF986u
#define RB240_AMBER    0xFD20u
#define RB240_CYAN     0x2D7Fu
#define RB240_GREEN    0x4FE8u

static uint32_t refm_pct(uint32_t v) { return (v * 100u + 16383u) / 32767u; }

static void refm_screw(int32_t x, int32_t y)
{
    lcd_fill(x, y, 5, 5, RB240_DARK);
    lcd_fill(x + 1, y + 1, 3, 3, RB240_PANEL2);
    lcd_fill(x + 2, y, 1, 5, RB240_LIGHT);
    lcd_fill(x, y + 2, 5, 1, RB240_LIGHT);
}

static void refm_panel(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t fill)
{
    int32_t yy;
    lcd_fill(x, y, w, h, fill);
    lcd_fill(x, y, w, 2, RB240_LIGHT);
    lcd_fill(x, y, 2, h, RB240_LIGHT);
    lcd_fill(x, y + h - 2, w, 2, RB240_DARK);
    lcd_fill(x + w - 2, y, 2, h, RB240_DARK);
    for (yy = y + 4; yy < y + h - 3; yy += 5)
        lcd_fill(x + 3, yy, w - 6, 1, (yy & 1) ? RB240_PANEL2 : fill);
    if (w >= 24 && h >= 18) {
        refm_screw(x + 3, y + 3);
        refm_screw(x + w - 8, y + 3);
    }
}

static void refm_band(int32_t y, const char *label, uint16_t color)
{
    cv_begin(232, 10, color);
    cv_rect(0, 0, 232, 1, RB240_LIGHT);
    cv_rect(0, 9, 232, 1, RB240_DARK);
    cv_text(7, 2, &AF_S, label, RB240_INK);
    cv_blit(4, y);
    refm_screw(6, y + 2);
    refm_screw(229, y + 2);
}

static void refm_led_dot(int32_t x, int32_t y, uint8_t on, uint16_t color)
{
    lcd_fill(x, y, 5, 5, RB240_DARK);
    lcd_fill(x + 1, y + 1, 3, 3, on ? color : RB240_LED_BG);
}

static void refm_knob(int32_t x, int32_t y, uint16_t value, uint16_t color)
{
    int32_t px = x + 6;
    (void)value;
    lcd_fill(x + 2, y, 10, 14, RB240_BLACK);
    lcd_fill(x, y + 2, 14, 10, RB240_BLACK);
    lcd_fill(x + 3, y + 3, 8, 8, RB240_DARK);
    lcd_fill(px, y + 2, 2, 6, color);
}

static void refm_meter_seg(int32_t x, int32_t y, int32_t h, uint32_t pct)
{
    uint32_t i;
    int32_t seg_h = 3, gap = 1;
    int32_t segs = h / (seg_h + gap);
    if (pct > 100u) pct = 100u;
    for (i = 0; i < (uint32_t)segs; ++i) {
        int32_t sy = y + h - (int32_t)(i + 1u) * (seg_h + gap);
        uint32_t threshold = (i * 100u) / (uint32_t)segs;
        uint16_t c = i >= (uint32_t)(segs - 2) ? RB240_RED : i >= (uint32_t)(segs - 4) ? RB240_AMBER : RB240_GREEN;
        lcd_fill(x, sy, 6, seg_h, pct > threshold ? c : RB240_DARK);
    }
}

static void refm_button(int32_t x, int32_t y, int32_t w, int32_t h, const char *label, uint8_t on, uint16_t color)
{
    cv_begin(w, h, RB240_PANEL);
    cv_rect(0, 0, w, 1, RB240_LIGHT);
    cv_rect(0, h - 1, w, 1, RB240_DARK);
    cv_rect(1, 1, w - 2, h - 2, on ? color : RB240_PANEL2);
    cv_text(3, h > 14 ? 4 : 2, &AF_S, label, on ? RB240_INK : RB240_TEXT);
    cv_blit(x, y);
}

static void refm_small_val(int32_t x, int32_t y, int32_t w, const char *label, int32_t value)
{
    char b[12];
    cv_begin(w, 34, RB240_PANEL);
    cv_rect(0, 0, w, 1, RB240_LIGHT);
    cv_rect(0, 33, w, 1, RB240_DARK);
    cv_text(4, 3, &AF_S, label, RB240_INK);
    cv_rect(4, 15, w - 8, 15, RB240_LED_BG);
    fmt_int(b, value);
    cv_text(7, 17, &AF_S, b, RB240_RED);
    cv_blit(x, y);
}

static void refm_small_text(int32_t x, int32_t y, int32_t w, const char *label, const char *value)
{
    cv_begin(w, 34, RB240_PANEL);
    cv_rect(0, 0, w, 1, RB240_LIGHT);
    cv_rect(0, 33, w, 1, RB240_DARK);
    cv_text(4, 3, &AF_S, label, RB240_INK);
    cv_rect(4, 15, w - 8, 15, RB240_LED_BG);
    cv_text(7, 17, &AF_S, value, RB240_RED);
    cv_blit(x, y);
}
''',
    )

    replace_once(
        draw,
        '''static void refm_card_val(int32_t x, int32_t y, const char *label, int32_t value)\n{\n    char b[12];\n    cv_begin(56, 44, T_SURF);\n    cv_text(4, 3, &AF_S, label, T_MID);\n    fmt_int(b, value);\n    cv_text(4, 21, &AF_M, b, T_THEME);\n    cv_blit(x, y);\n}\n''',
        '''static void refm_card_val(int32_t x, int32_t y, const char *label, int32_t value)\n{\n    char b[12];\n    cv_begin(56, 44, RB240_PANEL);\n    cv_rect(0, 0, 56, 2, RB240_LIGHT);\n    cv_rect(0, 42, 56, 2, RB240_DARK);\n    cv_text(5, 3, &AF_S, label, RB240_INK);\n    cv_rect(5, 18, 46, 20, RB240_LED_BG);\n    fmt_int(b, value);\n    cv_text(8, 21, &AF_M, b, RB240_RED);\n    cv_blit(x, y);\n}\n''',
    )
    replace_once(
        draw,
        '''static void refm_card_text(int32_t x, int32_t y, const char *label, const char *value)\n{\n    cv_begin(56, 44, T_SURF);\n    cv_text(4, 3, &AF_S, label, T_MID);\n    cv_text(4, 21, &AF_M, value, T_THEME);\n    cv_blit(x, y);\n}\n''',
        '''static void refm_card_text(int32_t x, int32_t y, const char *label, const char *value)\n{\n    cv_begin(56, 44, RB240_PANEL);\n    cv_rect(0, 0, 56, 2, RB240_LIGHT);\n    cv_rect(0, 42, 56, 2, RB240_DARK);\n    cv_text(5, 3, &AF_S, label, RB240_INK);\n    cv_rect(5, 18, 46, 20, RB240_LED_BG);\n    cv_text(8, 21, &AF_M, value, RB240_RED);\n    cv_blit(x, y);\n}\n''',
    )
    replace_once(
        draw,
        '''static void refm_footer(const char *text)\n{\n    cv_begin(240, 28, T_BG);\n    cv_text(5, 7, &AF_S, text, T_MID);\n    cv_blit(0, 212);\n}\n''',
        '''static void refm_footer(const char *text)\n{\n    cv_begin(240, 28, RB240_PANEL2);\n    cv_rect(0, 0, 240, 2, RB240_LIGHT);\n    cv_rect(4, 5, 232, 17, RB240_DARK);\n    cv_text(7, 8, &AF_S, text, RB240_TEXT);\n    cv_blit(0, 212);\n}\n''',
    )
    replace_once(
        draw,
        '''        uint16_t c = i == selected ? T_ACCENT : i == playing ? T_THEME : T_SURF;\n        lcd_fill(x, y, 22, 18, c);\n''',
        '''        uint16_t c = i == selected ? RB240_AMBER : i == playing ? RB240_GREEN : RB240_PANEL2;\n        lcd_fill(x, y, 22, 18, RB240_DARK);\n        lcd_fill(x + 2, y + 2, 18, 14, RB240_PANEL);\n        lcd_fill(x + 4, y + 4, 14, 10, c);\n''',
    )

    replace_once(
        draw,
        '''    lcd_fill(0, 0, 240, 240, T_BG);\n    cv_begin(240, 24, T_BG);\n    cv_text(5, HEAD_MY, &AF_M, pn, T_THEME);\n    cv_text(76, HEAD_MY, &AF_M, mn, T_TEXT);\n    fmt_int(b, bpm);\n    cv_text(177, HEAD_MY, &AF_M, b, T_THEME);\n    cv_text(213, HEAD_SY, &AF_S, playing ? "PLAY" : "STOP", playing ? T_ACCENT : T_MID);\n    cv_blit(0, 0);\n\n    cv_begin(240, 22, T_BG);\n    cv_text(5, 5, &AF_S, "PAT", T_MID);\n    b[0] = (char)('A' + (pattern & 7u)); b[1] = 0;\n    cv_text(34, 5, &AF_S, b, T_THEME);\n    cv_text(72, 5, &AF_S, "PRESET=PAT  ALGO=TRACK  SELECT=BPM", T_MID);\n    cv_blit(0, 27);\n''',
        '''    lcd_fill(0, 0, 240, 240, RB240_BG);\n    refm_panel(2, 2, 236, 23, RB240_PANEL);\n    lcd_fill(5, 5, 58, 17, RB240_AMBER);\n    cv_begin(56, 15, RB240_AMBER); cv_text(4, 1, &AF_M, pn, RB240_INK); cv_blit(6, 6);\n    cv_begin(66, 15, RB240_PANEL); cv_text(4, 1, &AF_M, mn, RB240_INK); cv_blit(65, 6);\n    lcd_fill(133, 5, 48, 17, RB240_LED_BG);\n    cv_begin(46, 15, RB240_LED_BG); fmt_int(b, bpm); cv_text(8, 1, &AF_M, b, RB240_RED); cv_blit(134, 6);\n    cv_begin(51, 15, RB240_DARK); cv_text(4, 2, &AF_S, playing ? "PLAY" : "STOP", playing ? RB240_GREEN : RB240_TEXT); cv_blit(184, 6);\n\n    refm_panel(2, 27, 236, 22, RB240_PANEL);\n    cv_begin(42, 15, RB240_PANEL); cv_text(4, 2, &AF_S, "PATTERN", RB240_INK); cv_blit(5, 31);\n    lcd_fill(48, 31, 17, 15, RB240_LED_BG);\n    b[0] = (char)('A' + (pattern & 7u)); b[1] = 0;\n    cv_begin(15, 13, RB240_LED_BG); cv_text(4, 1, &AF_S, b, RB240_RED); cv_blit(49, 32);\n    refm_button(68, 31, 18, 15, "<", 0u, RB240_AMBER);\n    refm_button(88, 31, 18, 15, ">", 0u, RB240_AMBER);\n    cv_begin(126, 16, RB240_PANEL); cv_text(2, 1, &AF_S, "PRESET   ALGO   SELECT", RB240_INK); cv_blit(108, 30);\n    refm_led_dot(126, 42, 1u, RB240_RED); refm_led_dot(165, 42, 0u, RB240_RED); refm_led_dot(211, 42, 0u, RB240_RED);\n''',
    )

    # Approved HOME preview layout.
    replace_once(draw,
'''    if (page == 0u) {\n        refm_card_text(3, 61, "TRACK", mn);\n        b[0] = (char)('A' + (pattern & 7u)); b[1] = 0; refm_card_text(62, 61, "PATTERN", b);\n        refm_card_val(121, 61, "STEP", (int32_t)play_step + 1);\n        refm_card_text(180, 61, "STATE", playing ? "PLAY" : "STOP");\n        cv_begin(240, 42, T_BG); cv_text(8, 9, &AF_M, "Keys play selected ACID track", T_TEXT); cv_blit(0, 119);\n        refm_footer("ENV=A  LFO=B  ARP=DRUM  SEQ=STEP");\n''',
'''    if (page == 0u) {\n        uint32_t i;\n        refm_band(51, "SEQUENCER", RB240_AMBER);\n        refm_small_text(4, 63, 56, "TRACK", mn);\n        b[0] = (char)('A' + (pattern & 7u)); b[1] = 0; refm_small_text(62, 63, 56, "PATTERN", b);\n        refm_small_val(120, 63, 56, "STEP", (int32_t)play_step + 1);\n        refm_small_text(178, 63, 58, "STATE", playing ? "PLAY" : "STOP");\n        refm_button(4, 101, 34, 20, "PREV", 0u, RB240_AMBER);\n        refm_button(40, 101, 34, 20, "NEXT", 0u, RB240_AMBER);\n        refm_button(76, 101, 34, 20, "PLAY", playing != 0u, RB240_GREEN);\n        refm_button(112, 101, 34, 20, "STOP", playing == 0u, RB240_RED);\n        refm_small_val(150, 99, 40, "BAR", (int32_t)(play_step / 16u) + 1);\n        refm_small_val(192, 99, 44, "LENGTH", 16);\n        refm_panel(4, 127, 146, 35, RB240_PANEL);\n        cv_begin(134, 10, RB240_PANEL); cv_text(5, 1, &AF_S, "TRACK MUTE", RB240_INK); cv_blit(8, 130);\n        for (i = 0; i < 8u; ++i) refm_button(9 + (int32_t)i * 17, 143, 14, 14, "", i == machine, RB240_AMBER);\n        refm_panel(154, 127, 82, 35, RB240_PANEL);\n        cv_begin(70, 10, RB240_PANEL); cv_text(4, 1, &AF_S, "LOOP", RB240_INK); cv_blit(158, 130);\n        refm_led_dot(161, 145, 1u, RB240_RED); refm_knob(183, 143, 0u, RB240_AMBER); refm_knob(213, 143, 0u, RB240_AMBER);\n        refm_panel(4, 168, 232, 30, RB240_PANEL2);\n        cv_begin(224, 18, RB240_DARK); cv_text(5, 5, &AF_S, "Keys play selected ACID track", RB240_TEXT); cv_blit(8, 174);\n        refm_footer("ENV=A  LFO=B  ARP=DRUM  SEQ=STEP");\n''')

    # ACID retains the beta semantics but adopts the approved material language.
    replace_once(draw, "    } else if (page == 1u) {\n", "    } else if (page == 1u) {\n        refm_band(51, \"ACID CONTROL\", RB240_AMBER);\n")
    replace_once(draw, "    } else if (page == 2u) {\n", "    } else if (page == 2u) {\n        refm_band(51, \"STEP PROGRAMMER\", RB240_AMBER);\n")

    # Approved DRUM preview layout.
    replace_once(draw,
'''    } else if (page == 3u) {\n        refm_steps(sel_step, playing ? play_step : 0xffffu);\n        refm_card_text(3, 139, "LANE", lane < 11u ? REFM_DRUM_NAME[lane] : "?");\n        refm_card_text(62, 139, "STATE", dstate == 0u ? "OFF" : dstate == 1u ? "HIT" : "ACC");\n        refm_card_val(121, 139, "STEP", (int32_t)sel_step + 1);\n        refm_card_text(180, 139, "KIT", machine == 2u ? "808" : "909");\n        refm_footer("K1 LANE K2 OFF/HIT/ACC | PRESET STEP");\n''',
'''    } else if (page == 3u) {\n        uint32_t i;\n        refm_band(51, "DRUM MACHINE", RB240_AMBER);\n        refm_panel(4, 63, 184, 51, RB240_PANEL);\n        for (i = 0; i < 16u; ++i) {\n            int32_t sx = 8 + (int32_t)(i & 7u) * 22;\n            int32_t sy = 70 + (int32_t)(i >> 3) * 22;\n            refm_button(sx, sy, 18, 17, "", i == sel_step || (playing && i == play_step), i == sel_step ? RB240_AMBER : RB240_GREEN);\n        }\n        refm_panel(191, 63, 45, 51, RB240_PANEL);\n        refm_small_val(195, 67, 37, "STEP", (int32_t)sel_step + 1);\n        refm_led_dot(198, 103, 0u, RB240_RED); refm_led_dot(218, 103, playing != 0u, RB240_RED);\n        refm_small_text(4, 119, 45, "LANE", lane < 11u ? REFM_DRUM_NAME[lane] : "?");\n        refm_small_text(51, 119, 45, "STATE", dstate == 0u ? "OFF" : dstate == 1u ? "HIT" : "ACC");\n        refm_small_val(98, 119, 45, "STEP", (int32_t)sel_step + 1);\n        refm_small_text(145, 119, 43, "KIT", machine == 2u ? "808" : "909");\n        refm_panel(191, 119, 45, 76, RB240_DARK);\n        cv_begin(38, 10, RB240_DARK); cv_text(2, 1, &AF_S, "OUTPUT", RB240_TEXT); cv_blit(195, 123);\n        refm_meter_seg(200, 137, 49, dstate == 0u ? 8u : dstate == 1u ? 64u : 100u);\n        refm_meter_seg(209, 137, 49, playing ? 76u : 22u);\n        refm_panel(4, 158, 184, 37, RB240_PANEL);\n        cv_begin(180, 9, RB240_PANEL); cv_text(4, 1, &AF_S, "TUNE   LEVEL   DECAY   TONE   SNAP   PAN", RB240_INK); cv_blit(6, 160);\n        for (i = 0; i < 6u; ++i) refm_knob(11 + (int32_t)i * 29, 174, 0u, RB240_AMBER);\n        refm_footer("K1 LANE  K2 OFF/HI/ACC  |  PRESET STEP");\n''')

    # Approved MIXER preview layout.
    replace_once(draw,
'''    } else if (page == 4u) {\n        refm_card_val(3, 61, "LEVEL", (int32_t)refm_pct(p0));\n        refm_card_val(62, 61, "PAN", ((int32_t)p1 - 32767) * 100 / 32767);\n        refm_card_val(121, 61, "DELAY", (int32_t)p2 * 100 / 127);\n        refm_card_val(180, 61, "REVERB", (int32_t)p3 * 100 / 127);\n        refm_card_text(3, 112, "MUTE", mute ? "ON" : "OFF");\n        refm_card_text(62, 112, "SOLO", solo ? "ON" : "OFF");\n        refm_card_text(121, 112, "EDIT", "MUTE");\n        refm_card_text(180, 112, "REC", "SOLO");\n        refm_footer("ALGO TRACK | K1 LVL K2 PAN K3 DLY K4 REV");\n''',
'''    } else if (page == 4u) {\n        refm_band(51, "CHANNEL / LEVELS", RB240_AMBER);\n        refm_small_val(4, 63, 41, "LEVEL", (int32_t)refm_pct(p0));\n        refm_small_val(47, 63, 41, "PAN", ((int32_t)p1 - 32767) * 100 / 32767);\n        refm_small_val(90, 63, 41, "DELAY", (int32_t)p2 * 100 / 127);\n        refm_small_val(133, 63, 41, "REVERB", (int32_t)p3 * 100 / 127);\n        refm_panel(4, 99, 41, 57, RB240_PANEL); refm_meter_seg(19, 107, 41, refm_pct(p0));\n        refm_panel(47, 99, 41, 57, RB240_PANEL); refm_knob(60, 117, p1, RB240_AMBER);\n        refm_panel(90, 99, 41, 57, RB240_PANEL); refm_knob(103, 117, p2, RB240_AMBER);\n        refm_panel(133, 99, 41, 57, RB240_PANEL); refm_knob(146, 117, p3, RB240_AMBER);\n        refm_panel(178, 63, 58, 93, RB240_DARK);\n        cv_begin(50, 10, RB240_DARK); cv_text(3, 1, &AF_S, "MASTER", RB240_TEXT); cv_blit(182, 67);\n        refm_meter_seg(187, 83, 57, refm_pct(p0)); refm_meter_seg(196, 83, 57, refm_pct(p0) > 8u ? refm_pct(p0) - 8u : 0u);\n        refm_knob(214, 126, p0, RB240_AMBER);\n        refm_small_text(4, 160, 41, "MUTE", mute ? "ON" : "OFF");\n        refm_small_text(47, 160, 41, "SOLO", solo ? "ON" : "OFF");\n        refm_small_text(90, 160, 41, "EDIT", "MUTE");\n        refm_small_text(133, 160, 41, "REC", "SOLO");\n        refm_footer("ALGO TRACK  |  K1 LVL  K2 PAN  K3 DLY  K4 REV");\n''')

    replace_once(draw, "    } else if (page == 5u) {\n", "    } else if (page == 5u) {\n        refm_band(51, \"MASTER FX\", RB240_AMBER);\n")

    # Approved SONG preview layout.
    replace_once(draw,
'''    } else if (page == 6u) {\n        refm_card_text(3, 61, "MODE", "PATTERN");\n        refm_card_text(62, 61, "BANK", "A-H");\n        refm_card_text(121, 61, "PLAY", playing ? "RUN" : "STOP");\n        refm_card_text(180, 61, "EDIT", "LOOP");\n        cv_begin(240, 42, T_BG); cv_text(8, 9, &AF_M, "Beta: pattern playback is primary", T_TEXT); cv_blit(0, 119);\n        refm_footer("PRESET PATTERN | PLAY START/STOP | EDIT LOOP");\n''',
'''    } else if (page == 6u) {\n        uint32_t i;\n        refm_band(51, "PATTERN / SONG", RB240_AMBER);\n        refm_small_text(4, 63, 56, "MODE", "PATTERN");\n        refm_small_text(62, 63, 56, "BANK", "A-H");\n        refm_small_text(120, 63, 56, "PLAY", playing ? "RUN" : "STOP");\n        refm_small_text(178, 63, 58, "EDIT", "LOOP");\n        refm_panel(4, 101, 232, 25, RB240_PANEL2);\n        cv_begin(224, 17, RB240_DARK); cv_text(5, 4, &AF_S, "Pattern playback / chain focus", RB240_TEXT); cv_blit(8, 105);\n        refm_button(4, 132, 34, 21, "PREV", 0u, RB240_AMBER);\n        refm_button(40, 132, 34, 21, "PLAY", playing != 0u, RB240_GREEN);\n        refm_button(76, 132, 34, 21, "STOP", playing == 0u, RB240_RED);\n        refm_button(112, 132, 34, 21, "REC", 0u, RB240_RED);\n        refm_small_val(150, 130, 40, "BAR", (int32_t)(play_step / 16u) + 1);\n        refm_small_val(192, 130, 44, "LENGTH", 16);\n        refm_panel(4, 160, 232, 40, RB240_PANEL);\n        cv_begin(224, 10, RB240_PANEL); cv_text(4, 1, &AF_S, "CHAIN", RB240_INK); cv_blit(8, 163);\n        for (i = 0; i < 16u; ++i) refm_button(8 + (int32_t)i * 14, 177, 12, 17, "", i == pattern, RB240_AMBER);\n        refm_footer("PRESET PATTERN  |  PLAY START/STOP  |  EDIT LOOP");\n''')

    replace_once(draw, "    } else if (page == 7u) {\n", "    } else if (page == 7u) {\n        refm_band(51, \"PROJECT MEMORY\", RB240_AMBER);\n")

    marker = root / ".refm-rb240-skin"
    marker.write_text("style=rb240-approved-preview-v3\nvirtual-contract=web/app/rb240_renderer.js\n")
    print("RB-240 approved preview hardware skin v3 applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
