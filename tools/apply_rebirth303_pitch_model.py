#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Final target-only ReBirth/TB-303 pitch interaction correction.

On the ACID STEP page the FM-1's physical OCT-/OCT+ buttons become the
ReBirth DOWN/UP controls: one press changes the selected step by exactly one
octave. Normal K1 editing remains semitone pitch selection inside C1..C4.
Other pages retain the FM-1 contextual OCT behavior installed by the hardware
rework pass.
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

    ui = root / "firmware" / "src" / "ui_input.c"
    replace_once(
        ui,
        "        if (page == 2u) refm_felucca_ui_encoder(0u, -1);      /* STEP pitch - */\n",
        "        if (page == 2u) { uint8_t rb303_i; for (rb303_i = 0u; rb303_i < 12u; ++rb303_i) refm_felucca_ui_encoder(0u, -1); } /* ReBirth DOWN */\n",
    )
    replace_once(
        ui,
        "        if (page == 2u) refm_felucca_ui_encoder(0u, 1);       /* STEP pitch + */\n",
        "        if (page == 2u) { uint8_t rb303_i; for (rb303_i = 0u; rb303_i < 12u; ++rb303_i) refm_felucca_ui_encoder(0u, 1); }  /* ReBirth UP */\n",
    )

    (root / STAMP).write_text(
        "model=rebirth-rb338-tb303\nstep-range-midi=24..60\nstep-octave=down-normal-up\n"
    )
    print("ReBirth/TB-303 physical pitch model applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
