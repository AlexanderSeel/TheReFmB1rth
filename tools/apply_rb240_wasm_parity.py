#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bring the final hardware-safe RB-240 renderer back toward WASM v2 parity.

The beta.4 hardware rework owns dirty-region drawing and must remain the base.
This narrow final pass changes presentation only: WASM page naming, a compact
16-step ACID strip, the 8x2 drum grid proportions, and contextual FX labels.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(text: str, old: str, new: str, what: str) -> str:
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"WASM parity anchor mismatch ({what}): expected 1, got {n}")
    return text.replace(old, new, 1)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    draw = args.felucca.resolve() / "firmware" / "src" / "ui_draw.c"
    text = draw.read_text()

    text = replace_once(
        text,
        'static const char *const RB5_PAGE[9] = {"HOME","ACID","STEP","DRUM","MIX","FX","SONG","SAVE","EXT"};',
        'static const char *const RB5_PAGE[9] = {"HOME","ACID","SEQ EDIT","DRUM","MIXER","FX","SONG","PROJECT","EXT"};',
        "page names",
    )
    text = text.replace('"RES",(int32_t)rb5_pct(p1)', '"RESO",(int32_t)rb5_pct(p1)')
    text = text.replace('"ENV",(int32_t)rb5_pct(p2)', '"ENV.MOD",(int32_t)rb5_pct(p2)')

    old_drum = '''static void rb5_drum_cell(uint32_t i, uint32_t packed, uint16_t selected, uint16_t playing)\n{\n    uint8_t state = (uint8_t)(packed & 3u);\n    uint8_t flam = (uint8_t)((packed & 4u) != 0u);\n    int32_t x = 4 + (int32_t)(i & 7u) * 29;\n    int32_t y = 61 + (int32_t)(i >> 3) * 39;\n    uint16_t fill = state == 2u ? RB5_RED : state == 1u ? RB5_AMBER : RB5_PANEL;\n    char b[4];\n    cv_begin(27, 34, RB5_PANEL2);\n    cv_rect(0, 0, 27, 1, i == selected ? RB5_TEXT : RB5_DARK);\n    cv_rect(0, 33, 27, 1, i == selected ? RB5_TEXT : RB5_DARK);\n    cv_rect(2, 3, 23, 25, fill);\n    b[0] = state == 2u ? 'A' : state == 1u ? 'X' : '-'; b[1] = 0;\n    cv_text(9, 9, &AF_S, b, state ? RB5_INK : RB5_DIM);\n    if (flam) cv_rect(4, 29, 19, 2, RB5_CYAN);\n    if (i == playing) cv_rect(24, 3, 2, 27, RB5_GREEN);\n    cv_blit(x, y);\n}\n\n'''
    new_drum = '''static void rb5_drum_cell(uint32_t i, uint32_t packed, uint16_t selected, uint16_t playing)\n{\n    uint8_t state = (uint8_t)(packed & 3u);\n    uint8_t flam = (uint8_t)((packed & 4u) != 0u);\n    int32_t x = 7 + (int32_t)(i & 7u) * 28;\n    int32_t y = 104 + (int32_t)(i >> 3) * 27;\n    uint16_t fill = state == 2u ? RB5_RED : state == 1u ? RB5_AMBER : RB5_PANEL;\n    char b[4];\n    cv_begin(24, 22, RB5_DARK);\n    cv_rect(1, 1, 22, 20, i == selected ? RB5_TEXT : RB5_DARK);\n    cv_rect(2, 2, 20, 18, fill);\n    fmt_int(b, (int32_t)i + 1);\n    cv_text(6, 5, &AF_S, b, state ? RB5_INK : RB5_TEXT);\n    if (flam) cv_rect(18, 3, 3, 3, RB5_CYAN);\n    if (i == playing) { cv_rect(1, 1, 22, 1, RB5_GREEN); cv_rect(1, 20, 22, 1, RB5_GREEN); }\n    cv_blit(x, y);\n}\n\nstatic void rb6_acid_strip_cell(uint32_t i, uint32_t packed, uint16_t selected, uint16_t playing)\n{\n    uint8_t flags = (uint8_t)((packed >> 8) & 0xffu);\n    uint8_t gate = (uint8_t)((flags & 1u) != 0u);\n    uint8_t acc = (uint8_t)((flags & 2u) != 0u);\n    uint8_t slide = (uint8_t)((flags & 4u) != 0u);\n    uint8_t tie = (uint8_t)((flags & 8u) != 0u && !gate);\n    int32_t x = 5 + (int32_t)i * 14;\n    uint16_t fill = acc && gate ? RB5_RED : gate ? RB5_AMBER : tie ? RB5_CYAN : RB5_DARK;\n    cv_begin(12, 18, RB5_DARK);\n    cv_rect(1, 1, 10, 16, fill);\n    if (slide) cv_rect(9, 3, 1, 10, RB5_TEXT);\n    if (i == selected) { cv_rect(0, 0, 12, 1, RB5_TEXT); cv_rect(0, 17, 12, 1, RB5_TEXT); }\n    if (i == playing) cv_rect(10, 2, 1, 14, RB5_GREEN);\n    cv_blit(x, 184);\n}\n\n'''
    text = replace_once(text, old_drum, new_drum, "drum/acid strip helpers")

    acid_tail = '''        if (force) {\n            rb5_textbox(62,118,56,42,"SEQ","STEP",RB5_RED);\n            rb5_textbox(121,118,56,42,"KEYS","LIVE",RB5_GREEN);\n            rb5_textbox(180,118,56,42,"OCT","-/+",RB5_RED);\n        }\n    } else if (page == 2u) {\n'''
    acid_tail_new = '''        if (force) {\n            rb5_textbox(62,118,56,42,"SEQ","STEP",RB5_RED);\n            rb5_textbox(121,118,56,42,"KEYS","LIVE",RB5_GREEN);\n            rb5_textbox(180,118,56,42,"OCT","-/+",RB5_RED);\n        }\n        for (i = 0u; i < 16u; ++i) {\n            uint32_t packed = refm_felucca_pattern_step((uint8_t)i);\n            if (force || packed != rb5.step[i] || i == sel_step || i == rb5.selected_step ||\n                (playing && (i == play_step || i == rb5.play_step)))\n                rb6_acid_strip_cell(i, packed, sel_step, playing ? play_step : 0xffffu);\n            rb5.step[i] = packed;\n        }\n    } else if (page == 2u) {\n'''
    text = replace_once(text, acid_tail, acid_tail_new, "acid strip")

    text = text.replace('rb5_textbox(3,145,56,42,"LANE"', 'rb5_textbox(3,163,48,30,"LANE"')
    text = text.replace('rb5_textbox(62,145,56,42,"STATE"', 'rb5_textbox(54,163,48,30,"STATE"')
    text = text.replace('rb5_numbox(121,145,56,42,"STEP"', 'rb5_numbox(104,163,58,30,"STEP"')
    text = text.replace('rb5_textbox(180,145,56,42,"KIT"', 'rb5_textbox(164,163,72,30,"KIT"')
    text = text.replace('rb5_textbox(3,194,56,42,"KEY","HIT"', 'rb5_textbox(3,198,56,38,"KEY","HIT"')
    text = text.replace('rb5_textbox(62,194,56,42,"OCT","LANE"', 'rb5_textbox(62,198,56,38,"OCT","LANE"')
    text = text.replace('rb5_textbox(121,194,56,42,"K2","LEVEL"', 'rb5_textbox(121,198,56,38,"K2","LEVEL"')
    text = text.replace('rb5_textbox(180,194,56,42,"K3","FLAM"', 'rb5_textbox(180,198,56,38,"K3","FLAM"')

    old_fx = '''        if (force || fx_section != rb5.fx_section) rb5_textbox(3,66,56,42,"SECTION",fx_section==0u?"PCF":fx_section==1u?"DELAY":fx_section==2u?"DIST":"COMP",RB5_RED);\n        if (force || p0 != rb5.p0) rb5_numbox(62,66,56,42,"K1",(int32_t)rb5_pct(p0),RB5_RED);\n        if (force || p1 != rb5.p1) rb5_numbox(121,66,56,42,"K2",(int32_t)rb5_pct(p1),RB5_RED);\n        if (force || p2 != rb5.p2) rb5_numbox(180,66,56,42,"K3",(int32_t)rb5_pct(p2),RB5_RED);\n        if (force || p3 != rb5.p3) rb5_numbox(3,118,56,42,"K4",(int32_t)rb5_pct(p3),RB5_RED);\n        if (force) rb5_textbox(62,118,174,42,"ALGO","FX SECTION",RB5_CYAN);\n'''
    new_fx = '''        if (force || fx_section != rb5.fx_section) rb5_textbox(3,66,56,42,"SECTION",fx_section==0u?"PCF":fx_section==1u?"DELAY":fx_section==2u?"DIST":"COMP",RB5_CYAN);\n        if (force || p0 != rb5.p0 || fx_section != rb5.fx_section) rb5_numbox(62,66,56,42,fx_section==0u?"CUTOFF":fx_section==1u?"TIME":fx_section==2u?"DRIVE":"AMOUNT",(int32_t)rb5_pct(p0),RB5_RED);\n        if (force || p2 != rb5.p2 || fx_section != rb5.fx_section) rb5_numbox(121,66,56,42,fx_section==1u?"FDBK":"--",(int32_t)rb5_pct(p2),RB5_RED);\n        if (force || p3 != rb5.p3 || fx_section != rb5.fx_section) rb5_numbox(180,66,56,42,fx_section==1u?"MIX":"--",(int32_t)rb5_pct(p3),RB5_RED);\n        if (force) {\n            rb5_textbox(3,118,56,42,"ALGO","SELECT",RB5_CYAN);\n            rb5_textbox(62,118,174,42,"MIXER","DLY/REV SENDS",RB5_GREEN);\n        }\n'''
    text = replace_once(text, old_fx, new_fx, "FX labels")

    draw.write_text(text)
    print("RB-240 physical renderer aligned toward WASM v2 layout")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
