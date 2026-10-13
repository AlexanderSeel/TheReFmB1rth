#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the FM-1 OCT-/OCT+ buttons useful inside ReFm editors.

On the physical STEP page they edit the selected ACID note by one semitone.
On the DRUM page they move the selected drum lane down/up. PRESET remains the
selected-step control on both pages. This matches the visible DN/UP affordance
and avoids requiring any modifier layer for basic pattern programming.
"""
from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"hardware-step-navigation anchor mismatch in {path}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("felucca", type=Path)
    args = parser.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-keyboard-performance").is_file():
        raise SystemExit("apply keyboard performance overlay before step navigation")

    ui = root / "firmware" / "src" / "ui_input.c"
    replace_once(
        ui,
        '''    if (RB_HIT(B_OCTDN)) {\n        if (page == 7u) refm_felucca_ui_action(12u);\n        else refm_felucca_ui_encoder(5u, -1);\n    }\n    if (RB_HIT(B_OCTUP)) {\n        if (page == 7u) refm_felucca_ui_action(13u);\n        else refm_felucca_ui_encoder(5u, 1);\n    }\n''',
        '''    if (RB_HIT(B_OCTDN)) {\n        if (page == 2u) refm_felucca_ui_encoder(0u, -1);      /* STEP: note down */\n        else if (page == 3u) refm_felucca_ui_encoder(0u, -1); /* DRUM: lane down */\n        else if (page == 7u) refm_felucca_ui_action(12u);\n        else refm_felucca_ui_encoder(5u, -1);\n    }\n    if (RB_HIT(B_OCTUP)) {\n        if (page == 2u) refm_felucca_ui_encoder(0u, 1);       /* STEP: note up */\n        else if (page == 3u) refm_felucca_ui_encoder(0u, 1);  /* DRUM: lane up */\n        else if (page == 7u) refm_felucca_ui_action(13u);\n        else refm_felucca_ui_encoder(5u, 1);\n    }\n''',
    )

    (root / ".refm-hardware-step-navigation").write_text(
        "step-select=PRESET\n"
        "acid-note-entry=physical-keyboard\n"
        "acid-note-down-up=OCTDN,OCTUP\n"
        "drum-hit-entry=physical-keyboard\n"
        "drum-lane-down-up=OCTDN,OCTUP\n"
        "drum-step-select=PRESET\n"
    )
    print("FM-1 direct STEP note and DRUM lane up/down controls applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
