#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Finish the approved RB-240 beta3 layout after the base skin is applied.

This post-pass intentionally touches renderer text/geometry only. Runtime state
comes from refm_felucca_ui_value() and is shared with the WASM panel bridge.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"RB-240 beta3 anchor mismatch in {path}: {old[:120]!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    draw = root / "firmware" / "src" / "ui_draw.c"

    # Acid sequence editor: mini synth strip + pitch-mode keyboard + step lane.
    replace_once(
        draw,
        '''    } else if (page == 2u) {\n        refm_band(51, "STEP PROGRAMMER", RB240_AMBER);\n        refm_steps(sel_step, playing ? play_step : 0xffffu);\n        refm_card_val(3, 139, "NOTE", p0);\n        refm_card_text(62, 139, "TYPE", stype == 0u ? "REST" : stype == 1u ? "NOTE" : "TIE");\n        refm_card_text(121, 139, "FLAGS", acc && slide ? "A+S" : acc ? "ACC" : slide ? "SLIDE" : "--");\n        refm_card_val(180, 139, "PROB", p3);\n        refm_footer("PRESET STEP | K1 NOTE K2 TYPE K3 FLAGS K4 PROB");\n''',
        '''    } else if (page == 2u) {\n        uint32_t i;\n        uint16_t cut = refm_felucca_ui_value(29u), res = refm_felucca_ui_value(30u);\n        uint16_t env = refm_felucca_ui_value(31u), dec = refm_felucca_ui_value(32u);\n        int32_t tune = (int32_t)refm_felucca_ui_value(25u) - 64;\n        refm_band(51, "SEQ EDIT", RB240_AMBER);\n        refm_panel(4, 64, 232, 45, RB240_PANEL);\n        cv_begin(34, 10, RB240_PANEL); cv_text(3, 1, &AF_S, "WAVE", RB240_INK); cv_blit(7, 67);\n        refm_button(7, 79, 30, 17, wave ? "SQR" : "SAW", 1u, RB240_AMBER);\n        cv_begin(190, 9, RB240_PANEL); cv_text(2, 1, &AF_S, "TUNE  CUTOFF  RESO  ENV.MOD  DECAY  ACCENT", RB240_INK); cv_blit(42, 67);\n        refm_knob(44, 81, (uint16_t)(tune + 64), RB240_AMBER);\n        refm_knob(75, 81, cut, RB240_AMBER); refm_knob(106, 81, res, RB240_AMBER);\n        refm_knob(137, 81, env, RB240_AMBER); refm_knob(168, 81, dec, RB240_AMBER);\n        refm_knob(199, 81, refm_felucca_ui_value(26u), RB240_AMBER);\n        refm_panel(4, 112, 232, 51, RB240_PANEL);\n        cv_begin(37, 35, RB240_DARK); cv_text(5, 4, &AF_S, "PITCH", RB240_TEXT); cv_text(6, 15, &AF_S, "MODE", RB240_TEXT); cv_blit(7, 118);\n        refm_led_dot(20, 145, stype == 1u, RB240_RED);\n        refm_button(8, 150, 32, 10, "CLEAR", 0u, RB240_AMBER);\n        for (i = 0; i < 7u; ++i) {\n            int32_t kx = 46 + (int32_t)i * 18;\n            refm_button(kx, 129, 15, 29, "", 0u, RB240_PANEL);\n        }\n        lcd_fill(57, 118, 11, 22, RB240_BLACK); lcd_fill(75, 118, 11, 22, RB240_BLACK);\n        lcd_fill(111, 118, 11, 22, RB240_BLACK); lcd_fill(129, 118, 11, 22, RB240_BLACK); lcd_fill(147, 118, 11, 22, RB240_BLACK);\n        refm_led_dot(213, 126, acc != 0u, RB240_RED);\n        cv_begin(57, 16, RB240_PANEL); cv_text(4, 2, &AF_S, stype == 0u ? "REST" : stype == 2u ? "TIE" : "NOTE", RB240_INK); cv_blit(174, 118);\n        refm_button(174, 146, 13, 13, "DN", 0u, RB240_AMBER); refm_button(189, 146, 13, 13, "UP", 0u, RB240_AMBER);\n        refm_button(204, 146, 13, 13, "AC", acc != 0u, RB240_AMBER); refm_button(219, 146, 13, 13, "SL", slide != 0u, RB240_AMBER);\n        for (i = 0; i < 16u; ++i) {\n            int32_t sx = 5 + (int32_t)i * 14;\n            refm_button(sx, 178, 12, 18, "", i == sel_step || (playing && i == play_step), i == sel_step ? RB240_RED : RB240_GREEN);\n        }\n        refm_footer("K1 NOTE K2 TYPE K3 ACC/SLIDE K4 PROB | PRESET STEP");\n''',
    )

    # Distinguish the two rhythm machines in the already-approved drum layout.
    replace_once(draw, '        refm_band(51, "DRUM MACHINE", RB240_AMBER);\n',
                 '        refm_band(51, machine == 2u ? "TR-808 RHYTHM" : "TR-909 RHYTHM", RB240_AMBER);\n')
    replace_once(draw,
        '        refm_small_text(51, 119, 45, "STATE", dstate == 0u ? "OFF" : dstate == 1u ? "HIT" : "ACC");\n',
        '        refm_small_text(51, 119, 45, machine == 2u ? "STATE" : "LEVEL", dstate == 0u ? "OFF" : machine == 3u ? (dstate == 1u ? "LOW" : "HIGH") : (dstate == 1u ? "HIT" : "ACC"));\n')
    replace_once(draw,
        '        cv_begin(180, 9, RB240_PANEL); cv_text(4, 1, &AF_S, "TUNE   LEVEL   DECAY   TONE   SNAP   PAN", RB240_INK); cv_blit(6, 160);\n',
        '        cv_begin(180, 9, RB240_PANEL); cv_text(4, 1, &AF_S, machine == 2u ? "TUNE   LEVEL   DECAY   TONE   SNAP   PAN" : "TUNE   ATT     DECAY   TONE   SNAP   FLAM", RB240_INK); cv_blit(6, 160);\n')
    replace_once(draw,
        '        refm_footer("K1 LANE  K2 OFF/HI/ACC  |  PRESET STEP");\n',
        '        refm_footer(machine == 2u ? "K1 LANE K2 OFF/HIT/ACC | PRESET STEP" : "K1 LANE K2 OFF/LOW/HIGH | FLAM UI BETA");\n')

    # ReBirth-inspired FX rack: PCF, DELAY, DIST and COMP are selectable modules.
    replace_once(
        draw,
        '''    } else if (page == 5u) {\n        refm_band(51, "MASTER FX", RB240_AMBER);\n        refm_card_val(3, 61, "DRIVE", (int32_t)refm_pct(p0));\n        refm_card_val(62, 61, "DLY FB", (int32_t)p1 * 100 / 31000);\n        refm_card_val(121, 61, "REV MIX", (int32_t)refm_pct(p2));\n        refm_card_val(180, 61, "FILTER", (int32_t)refm_pct(p3));\n        refm_footer("K1 DRIVE K2 DELAY K3 REVERB K4 FILTER");\n''',
        '''    } else if (page == 5u) {\n        uint16_t fx = refm_felucca_ui_value(22u);\n        const char *fxn = fx == 0u ? "PCF" : fx == 1u ? "DELAY" : fx == 2u ? "DIST" : "COMP";\n        refm_band(51, fxn, RB240_CYAN);\n        refm_button(5, 64, 54, 18, "PCF", fx == 0u, RB240_CYAN);\n        refm_button(63, 64, 54, 18, "DELAY", fx == 1u, RB240_CYAN);\n        refm_button(121, 64, 54, 18, "DIST", fx == 2u, RB240_CYAN);\n        refm_button(179, 64, 54, 18, "COMP", fx == 3u, RB240_CYAN);\n        refm_panel(4, 86, 232, 108, RB240_PANEL);\n        if (fx == 0u) {\n            cv_begin(180, 10, RB240_PANEL); cv_text(5, 1, &AF_S, "PATTERN CONTROL FILTER", RB240_INK); cv_blit(10, 90);\n            refm_small_val(10, 104, 50, "FREQ", p0); refm_small_text(64, 104, 50, "MODE", "LP");\n            refm_small_val(118, 104, 50, "AMT", p2); refm_small_val(172, 104, 50, "DECAY", p3);\n            refm_knob(28, 157, p0, RB240_CYAN); refm_knob(82, 157, 35u, RB240_CYAN);\n            refm_knob(136, 157, p2, RB240_CYAN); refm_knob(190, 157, p3, RB240_CYAN);\n        } else if (fx == 1u) {\n            cv_begin(180, 10, RB240_PANEL); cv_text(5, 1, &AF_S, "TEMPO DELAY", RB240_INK); cv_blit(10, 90);\n            refm_small_val(10, 104, 50, "STEPS", p0); refm_small_val(64, 104, 50, "PAN", 0);\n            refm_small_val(118, 104, 50, "FEEDBACK", p2); refm_small_val(172, 104, 50, "MIX", p3);\n            refm_knob(28, 157, p0, RB240_CYAN); refm_knob(82, 157, 50u, RB240_CYAN);\n            refm_knob(136, 157, p2, RB240_CYAN); refm_knob(190, 157, p3, RB240_CYAN);\n        } else if (fx == 2u) {\n            cv_begin(180, 10, RB240_PANEL); cv_text(5, 1, &AF_S, "DISTORTION", RB240_INK); cv_blit(10, 90);\n            refm_small_val(20, 108, 85, "AMOUNT", p0); refm_small_val(132, 108, 85, "SHAPE", p1);\n            refm_knob(48, 158, p0, RB240_AMBER); refm_knob(160, 158, p1, RB240_AMBER);\n        } else {\n            cv_begin(180, 10, RB240_PANEL); cv_text(5, 1, &AF_S, "COMPRESSOR", RB240_INK); cv_blit(10, 90);\n            refm_small_val(20, 108, 85, "THRESH", p0); refm_small_val(132, 108, 85, "RATIO", p1);\n            refm_knob(48, 158, p0, RB240_GREEN); refm_knob(160, 158, p1 * 20u, RB240_GREEN);\n        }\n        refm_footer("FX/EDIT NEXT REC PREV | ALGO MODULE | K1-K4 EDIT");\n''',
    )

    marker = root / ".refm-rb240-beta3-layout"
    marker.write_text("style=rb240-beta3-approved\nseq=keyboard-strip\ndrums=808-909-distinct\nfx=pcf-delay-dist-comp\n")
    print("RB-240 beta3 SEQ/drum/FX parity layout applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
