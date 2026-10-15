#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Keep physical FM-1 output scaling neutral after master-bus headroom.

The shared mixer now reserves -12 dB before the optional nonlinear stages so a
four-track full-scale sum remains clean. The older hardware rework also applied
an extra fixed -6 dB after the master pot. Keeping both attenuators makes the
physical device needlessly quiet and hides where headroom is actually owned.
This narrow target pass makes the final hardware stage unity; the physical
MASTER pot remains active and the shared mixer owns summing headroom.
"""
from __future__ import annotations

import argparse
from pathlib import Path

STAMP = ".refm-hardware-clean-output"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-rework").is_file():
        raise SystemExit("apply hardware rework before clean output correction")

    audio = root / "firmware" / "src" / "audio.c"
    text = audio.read_text()
    old = "#define REFM_HW_HEADROOM_Q15 16384\n"
    new = "#define REFM_HW_HEADROOM_Q15 32767 /* shared mixer owns summing headroom */\n"
    if text.count(old) != 1:
        raise SystemExit("physical headroom anchor missing or duplicated")
    audio.write_text(text.replace(old, new, 1))
    (root / STAMP).write_text(
        "target-output-headroom=unity\n"
        "master-pot=preserved\n"
        "summing-headroom=shared-mixer\n"
    )
    print("FM-1 target output scaling set to unity after shared mixer headroom")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
