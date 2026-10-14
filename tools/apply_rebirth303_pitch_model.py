#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Final target-only FM-1 / TB-303 pitch interaction correction.

The Virtual ReBirth surface keeps its compact C..C + DOWN/UP interaction, but
physical FM-1 hardware has a real 27-key chromatic keyboard (F3..G5). Treating
those physical keys as a 13-key ReBirth membrane was a hardware UX bug: note
entry could store a pitch unrelated to the key that was actually pressed.

On hardware, OCT-/OCT+ therefore shift the physical keyboard register by one
octave on the ACID STEP page just as they do for live playing. K1 remains the
fine semitone edit for the selected step. This gives the controls distinct,
predictable jobs:

- musical keys: enter exactly the pitch printed/played by the FM-1 keyboard;
- OCT-/OCT+: move the physical keyboard down/up an octave;
- K1: fine-adjust the selected programmed note by semitone.
"""
from __future__ import annotations

import argparse
from pathlib import Path

STAMP = ".refm-rebirth303-pitch"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"FM-1 pitch anchor mismatch in {path}: expected 1, got {count}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-rework").is_file():
        raise SystemExit("apply hardware rework before FM-1 pitch model")

    ui = root / "firmware" / "src" / "ui_input.c"

    # apply_hardware_rework.py initially makes OCT-/OCT+ duplicate K1's
    # semitone edit on STEP. Replace that redundancy with the normal hardware
    # keyboard octave state. Key-up ownership is already retained by the audio
    # ISR scanner, so changing octave cannot leave a note stuck.
    replace_once(
        ui,
        "        if (page == 2u) refm_felucca_ui_encoder(0u, -1);      /* STEP pitch - */\n",
        "        if (page == 2u && song.octave > -3) { refm_felucca_panic(); song.octave--; } /* keyboard OCT- */\n",
    )
    replace_once(
        ui,
        "        if (page == 2u) refm_felucca_ui_encoder(0u, 1);       /* STEP pitch + */\n",
        "        if (page == 2u && song.octave < 3) { refm_felucca_panic(); song.octave++; }  /* keyboard OCT+ */\n",
    )

    (root / STAMP).write_text(
        "model=fm1-physical-chromatic+tb303-engine\n"
        "hardware-keyboard=F3..G5+octave\n"
        "hardware-step-entry=exact-midi-note\n"
        "hardware-octave=-3..+3\n"
        "hardware-k1=selected-step-semitone\n"
        "virtual-step-keyboard=rebirth-c-to-c-down-normal-up\n"
    )
    print("FM-1 physical chromatic / TB-303 pitch model applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
