#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Route the physical FM-1 keyboard directly into the ReFm performance model.

The first hardware validation showed that deferred tap-vs-hold page buttons were
far too confusing for basic sequencing. Physical controls are therefore kept
page-first and immediate: SEQ opens the ACID step editor, ARP opens drums, GLO
opens mixer, FX opens effects, etc.

The ReFm keyboard mapping is deliberately independent from Felucca's selected
track/engine. Using ``kb_map(TSEL, k)`` here was a hardware bug: ReFm can show
ACID A while Felucca's hidden selected track still owns a drum/quantized keymap,
which can turn a physical key into a GM drum note or even KB_SILENT. ReFm uses a
stable chromatic 27-key map instead, with Felucca's physical octave state only
providing the octave offset.

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
        "                uint32_t note = kb_map(TSEL, k);\n"
        "                uint8_t on = (cur & bit) ? 1u : 0u;\n"
        "                if (note <= 127u && (!on || (song.grid == 0u && !refm_felucca_keys_are_editor())))\n"
        "                    refm_felucca_key_note((uint8_t)song.sel, (uint8_t)note, on);\n",
        "                int32_t refm_note = 53 + (int32_t)k + 12 * (int32_t)song.octave;\n"
        "                uint32_t note = (uint32_t)clamp(refm_note, 0, 127);\n"
        "                uint8_t on = (cur & bit) ? 1u : 0u;\n"
        "                refm_felucca_key_event((uint8_t)k, (uint8_t)note, on);\n",
    )

    # apply_hardware_beta_fixes.py already installs immediate page/button actions
    # and direct K1-K4/PRESET/ALGO/SELECT encoder routing. Keep that simple input
    # contract intact rather than replacing it with deferred modifier ownership.
    marker = root / ".refm-keyboard-performance"
    marker.write_text(
        "keyboard=physical-index+dedicated-chromatic-note\n"
        "keyboard-base-midi=53\n"
        "keyboard-octave=physical-song.octave\n"
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
