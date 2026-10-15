#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Keep RB-240 display arithmetic from materializing ROM-shaped constants."""
from __future__ import annotations
import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    path = args.felucca.resolve() / "firmware" / "src" / "ui_draw.c"
    text = path.read_text()
    old = 'fmt_int(b, ((int32_t)pan - 32767) * 100 / 32767); cv_text(98, 7, &AF_S, b, RB5_TEXT);'
    new = 'fmt_int(b, ((int32_t)pan * 100 / 32767) - 100); cv_text(98, 7, &AF_S, b, RB5_TEXT);'
    if text.count(old) != 1:
        raise SystemExit(f"RB-240 pan arithmetic anchor mismatch: expected 1, got {text.count(old)}")
    path.write_text(text.replace(old, new, 1))
    print("RB-240 mixer pan display arithmetic made target ROM-safe")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
