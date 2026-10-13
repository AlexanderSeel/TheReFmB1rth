#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Patch the physical FM-1 beta overlay with keyboard-first ReFm interaction.

Runs after apply_hardware_beta_fixes.py. It deliberately uses Felucca only for
physical key/button scanning; interpretation lives in ui240_performance.h and
refm_felucca_bridge.c so WASM and hardware share the same musical behavior.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"keyboard-performance anchor mismatch in {path}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-beta").is_file():
        raise SystemExit("apply hardware beta fixes before keyboard performance overlay")

    seq = root / "firmware" / "src" / "seq.c"
    replace_once(
        seq,
        "extern void refm_felucca_key_note(uint8_t track, uint8_t note, uint8_t on);\n"
        "extern int refm_felucca_keys_are_editor(void);\n",
        "extern void refm_felucca_key_event(uint8_t key, uint8_t note, uint8_t on);\n",
    )
    replace_once(
        seq,
        "                if (note <= 127u && (!on || (song.grid == 0u && !refm_felucca_keys_are_editor())))\n"
        "                    refm_felucca_key_note((uint8_t)song.sel, (uint8_t)note, on);\n",
        "                if (note <= 127u)\n"
        "                    refm_felucca_key_event((uint8_t)k, (uint8_t)note, on);\n",
    )

    ui = root / "firmware" / "src" / "ui_input.c"
    replace_once(
        ui,
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\n\n"
        "static int refm_beta_input(uint32_t pressed)\n"
        "{\n"
        "    int32_t s;\n"
        "    uint16_t page = refm_felucca_ui_value(0u);\n"
        "#define RB_HIT(lbl) ((pressed & (1u << panel.btn[(lbl)])) != 0u)\n",
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\n"
        "extern void refm_felucca_ui_modifier(uint8_t modifier);\n\n"
        "static int refm_beta_input(uint32_t pressed)\n"
        "{\n"
        "    int32_t s;\n"
        "    uint8_t layer = 0u;\n"
        "    uint16_t page = refm_felucca_ui_value(0u);\n"
        "#define RB_HIT(lbl) ((pressed & (1u << panel.btn[(lbl)])) != 0u)\n"
        "#define RB_HELD(lbl) ((fm1_in.buttons & (1u << panel.btn[(lbl)])) != 0u)\n"
        "    /* Held layers are intentionally ordered from the most explicit\n"
        "       performance gesture to the broadest page shortcut. SCL/SEL is\n"
        "       the quick FOCUS navigator because its short press remains SONG. */\n"
        "    if (RB_HELD(B_SCL)) layer = 8u;\n"
        "    else if (RB_HELD(B_REC)) layer = 7u;\n"
        "    else if (RB_HELD(B_EDIT)) layer = 6u;\n"
        "    else if (RB_HELD(B_SAVE)) layer = 5u;\n"
        "    else if (RB_HELD(B_FX)) layer = 4u;\n"
        "    else if (RB_HELD(B_GLO)) layer = 3u;\n"
        "    else if (RB_HELD(B_ARP)) layer = 2u;\n"
        "    else if (RB_HELD(B_SEQ)) layer = 1u;\n"
        "    refm_felucca_ui_modifier(layer);\n",
    )
    replace_once(ui, "#undef RB_HIT\n", "#undef RB_HELD\n#undef RB_HIT\n")

    marker = root / ".refm-keyboard-performance"
    marker.write_text(
        "keyboard=physical-index+note\n"
        "layers=seq,drum,mixer,fx,pattern,edit,live,focus\n"
        "focus=hold-scl\n"
        "parity=ui240-performance\n"
    )
    print("FM-1 keyboard performance/modifier/focus overlay applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
