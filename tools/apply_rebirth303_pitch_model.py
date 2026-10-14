#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Final target-only ReBirth/TB-303 pitch interaction correction.

On the ACID STEP page the FM-1's physical OCT-/OCT+ buttons become ReBirth
DOWN/UP controls. They switch the selected step between a single C..C pitch
keyboard at DOWN / normal / UP, instead of behaving like generic octave
navigation. Other pages retain the contextual FM-1 behavior installed by the
hardware rework pass.
"""
from __future__ import annotations

import argparse
from pathlib import Path

STAMP = ".refm-rebirth303-pitch"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"ReBirth pitch anchor mismatch in {path}: expected 1, got {count}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-rework").is_file():
        raise SystemExit("apply hardware rework before ReBirth pitch model")

    bridge = root / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c"
    helper = r'''/* ReBirth/TB-303 step octave state. The pitch key itself is one C..C
 * octave; DOWN/UP transpose that selected pitch exactly one octave. */
void refm_felucca_step_octave(int8_t direction)
{
    seq16_step_t *st;
    int base;
    int old_mode;
    int new_mode;
    ensure_ready();
    if (!direction || g_ui.machine >= UI240_MACHINE_808 || g_ui.page != UI240_ACID_STEP)
        return;
    st = &g_refm.groovebox.acid_seq[g_ui.machine].step[g_ui.selected_step & 15u];
    old_mode = (st->flags & SEQ16_DOWN) ? -1 : (st->flags & SEQ16_UP) ? 1 :
               (st->note >= 24u && st->note < 36u) ? -1 :
               (st->note > 48u && st->note <= 60u) ? 1 : 0;
    base = (st->note >= 24u && st->note <= 60u) ? ((int)st->note - old_mode * 12) : 36;
    while (base < 36) base += 12;
    while (base > 48) base -= 12;
    new_mode = direction < 0 ? (old_mode == -1 ? 0 : -1) : (old_mode == 1 ? 0 : 1);
    st->flags &= (uint8_t)~(SEQ16_DOWN | SEQ16_UP);
    if (new_mode < 0) st->flags |= SEQ16_DOWN;
    if (new_mode > 0) st->flags |= SEQ16_UP;
    base += new_mode * 12;
    if (base < 24) base = 24;
    if (base > 60) base = 60;
    st->note = (uint8_t)base;
    if (!(st->flags & (SEQ16_GATE | SEQ16_TIE))) st->flags |= SEQ16_GATE;
    if (!st->probability) st->probability = 100u;
    groovebox_pattern_store(&g_refm.groovebox.patterns, &g_refm.groovebox,
                            g_refm.groovebox.current_pattern);
}

'''
    replace_once(
        bridge,
        "/* Backward-compatible entry point retained for older overlay tests. */\n",
        helper + "/* Backward-compatible entry point retained for older overlay tests. */\n",
    )

    ui = root / "firmware" / "src" / "ui_input.c"
    replace_once(
        ui,
        "    if (RB_HIT(B_OCTDN)) {\n",
        "    extern void refm_felucca_step_octave(int8_t direction);\n    if (RB_HIT(B_OCTDN)) {\n",
    )
    replace_once(
        ui,
        "        if (page == 2u) refm_felucca_ui_encoder(0u, -1);      /* STEP pitch - */\n",
        "        if (page == 2u) refm_felucca_step_octave(-1);         /* ReBirth DOWN */\n",
    )
    replace_once(
        ui,
        "        if (page == 2u) refm_felucca_ui_encoder(0u, 1);       /* STEP pitch + */\n",
        "        if (page == 2u) refm_felucca_step_octave(1);          /* ReBirth UP */\n",
    )

    (root / STAMP).write_text(
        "model=rebirth-rb338-tb303\nstep-range-midi=24..60\nstep-pitch-key=C..C\nstep-octave=down-normal-up\n"
    )
    print("ReBirth/TB-303 physical pitch model applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
