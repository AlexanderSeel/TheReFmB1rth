#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Replace Felucca's blocking text splash with a short non-blocking ReFmB1rth boot sequence.

The animation deliberately uses only existing LCD/text primitives and incremental
partial updates. Audio/USB/input/timer initialization completes before the
animated phases run in the normal main loop.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"ReFm boot splash anchor mismatch in {path}: {old[:120]!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    main_c = root / "firmware" / "src" / "main.c"

    helper = r'''static void refm_boot_base(void)
{
    uint32_t i;
    lcd_fill(0, 0, 240, 240, T_BG);
    lcd_fill(8, 8, 224, 224, T_MID);
    lcd_fill(13, 13, 214, 214, T_BG);
    lcd_fill(17, 17, 206, 27, T_MID);
    draw_text_line(22, 22, 32, &AF_S, "THE", T_TEXT, T_MID, 0);
    draw_text_line(48, 20, 78, &AF_M, "ReFM", T_THEME, T_MID, 0);
    draw_text_line(105, 20, 104, &AF_M, "B1RTH", T_THEME, T_MID, 0);
    draw_text_line(20, 52, 200, &AF_S, "M-VAVE FM-1 / REBORN GROOVE SYSTEM", T_MID, T_BG, 0);
    for (i = 0; i < 4u; ++i) {
        int32_t y = 79 + (int32_t)i * 25;
        lcd_fill(19, y, 202, 20, T_SURF);
        lcd_fill(25, y + 6, 7, 7, T_BG);
    }
    draw_text_line(39, 84, 80, &AF_S, "ACID A", T_TEXT, T_SURF, 0);
    draw_text_line(39, 109, 80, &AF_S, "ACID B", T_TEXT, T_SURF, 0);
    draw_text_line(39, 134, 80, &AF_S, "808", T_TEXT, T_SURF, 0);
    draw_text_line(39, 159, 80, &AF_S, "909", T_TEXT, T_SURF, 0);
    lcd_fill(18, 183, 204, 30, T_BG);
    draw_text_line(21, 186, 80, &AF_S, "16-STEP BUS", T_MID, T_BG, 0);
    for (i = 0; i < 16u; ++i)
        lcd_fill(21 + (int32_t)i * 12, 198, 9, 9, T_SURF);
    lcd_fill(18, 217, 204, 9, T_BG);
    draw_text_line(80, 217, 80, &AF_S, "PANEL BUS", T_THEME, T_BG, 1);
}

static void refm_boot_machine(uint32_t index, const char *status)
{
    int32_t y = 79 + (int32_t)index * 25;
    lcd_fill(25, y + 6, 7, 7, T_THEME);
    draw_text_line(156, y + 5, 58, &AF_S, status, T_THEME, T_SURF, 1);
}

static void refm_boot_status(const char *status)
{
    lcd_fill(18, 217, 204, 9, T_BG);
    draw_text_line(60, 217, 120, &AF_S, status, T_THEME, T_BG, 1);
}

static void refm_boot_tick(uint32_t elapsed)
{
    static uint32_t phase = 0xffffffffu, step = 0xffffffffu;
    uint32_t p = elapsed / 260u;
    uint32_t n = elapsed < 520u ? 0u : (elapsed - 520u) / 48u + 1u;
    uint32_t i;
    if (p > 5u) p = 5u;
    if (n > 16u) n = 16u;
    if (phase == 0xffffffffu) { phase = 0u; step = 0u; }
    while (phase < p) {
        ++phase;
        if (phase == 1u) { refm_boot_machine(0u, "ONLINE"); refm_boot_status("ACID A ENGINE"); }
        else if (phase == 2u) { refm_boot_machine(1u, "ONLINE"); refm_boot_status("ACID B ENGINE"); }
        else if (phase == 3u) { refm_boot_machine(2u, "ONLINE"); refm_boot_status("DRUM VOICES"); }
        else if (phase == 4u) { refm_boot_machine(3u, "ONLINE"); refm_boot_status("MIXER + FX"); }
        else if (phase == 5u) refm_boot_status("READY");
    }
    if (step > n) step = 0u;
    for (i = step; i < n; ++i)
        lcd_fill(22 + (int32_t)i * 12, 199, 7, 7, (phase >= 5u && (i & 3u) == 0u) ? T_ACCENT : T_THEME);
    step = n;
}

'''
    replace_once(
        main_c,
        "static void fm1_main(void)\n{\n    int32_t knob = 512 * 16;\n",
        helper + "static void fm1_main(void)\n{\n    int32_t knob = 512 * 16;\n    uint32_t refm_boot_t0;\n",
    )
    replace_once(
        main_c,
        "    lcd_fill(0, 0, 240, 240, T_BG);\n"
        "    draw_text_box(0, 94, 240, &AF_L, \"FELUCCA\", T_THEME, 1);\n"
        "    draw_text_box(0, 134, 240, &AF_S, \"MULTI-ENGINE SYNTH\", T_MID, 1);\n",
        "    refm_boot_base();\n",
    )
    replace_once(
        main_c,
        "    fm1_delay_ms(400);\n"
        "    lcd_fill(0, 0, 240, 240, T_BG);\n\n"
        "    for (;;) {\n",
        "    refm_boot_base();\n"
        "    refm_boot_t0 = fm1_ms;\n\n"
        "    for (;;) {\n",
    )
    replace_once(
        main_c,
        "        ui_leds();\n"
        "        ui_draw();\n"
        "        felucca_dbg.stage = 9;\n",
        "        ui_leds();\n"
        "        if (fm1_ms - refm_boot_t0 < 1560u)\n"
        "            refm_boot_tick(fm1_ms - refm_boot_t0);\n"
        "        else\n"
        "            ui_draw();\n"
        "        felucca_dbg.stage = 9;\n",
    )
    print("ReFmB1rth non-blocking hardware boot splash applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
