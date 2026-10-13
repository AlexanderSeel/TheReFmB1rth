#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Route the physical FM-1 keyboard directly into the ReFm performance model.

The first hardware validation showed that deferred tap-vs-hold page buttons were
far too confusing for basic sequencing. Physical controls are therefore kept
page-first and immediate: SEQ opens the ACID step editor, ARP opens drums, GLO
opens mixer, FX opens effects, etc. The musical keyboard still carries both its
physical key index and MIDI note into ui240_performance.h, so the active STEP or
DRUM page can program the selected step directly without a modifier gesture.

Advanced modifier/chord behavior remains available in Virtual FM-1; it is no
longer imposed on the hardware's primary page buttons.
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
    parser = argparse.ArgumentParser()
    parser.add_argument("felucca", type=Path)
    args = parser.parse_args()
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

    # apply_hardware_beta_fixes.py already installs immediate page/button actions
    # and direct K1-K4/PRESET/ALGO/SELECT encoder routing. Keep that simple input
    # contract intact rather than replacing it with deferred modifier ownership.
    marker = root / ".refm-keyboard-performance"
    marker.write_text(
        "keyboard=physical-index+note\n"
        "hardware-controls=immediate-page-first\n"
        "acid-step=page-direct-key-entry\n"
        "drum-step=page-direct-key-entry\n"
        "advanced-modifiers=virtual-only\n"
        "parity=ui240-performance-musical-state\n"
    )
    print("FM-1 direct page-first keyboard performance overlay applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
