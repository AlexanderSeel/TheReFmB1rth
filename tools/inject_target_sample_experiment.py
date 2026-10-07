#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inject generated sample assets into an already-applied Felucca overlay for link-size measurement.

This intentionally does not enable sample playback and does not flash/package a release.
It only adds the generated const PCM object to the target link so XIP cost can be measured.
"""
from __future__ import annotations
import argparse, shutil
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    ap.add_argument("assets", type=Path, help="directory containing refm_sample_assets.c/.h")
    args = ap.parse_args()
    dst = args.felucca.resolve()
    assets = args.assets.resolve()
    csrc = assets / "refm_sample_assets.c"
    hsrc = assets / "refm_sample_assets.h"
    if not csrc.is_file() or not hsrc.is_file():
        raise SystemExit("generated sample assets are missing")
    overlay = dst / ".refm-overlay"
    if not overlay.is_file():
        raise SystemExit("refusing sample experiment: ReFmB1rth overlay stamp is missing")
    gen = dst / "firmware" / "refm" / "generated"
    gen.mkdir(parents=True, exist_ok=True)
    shutil.copy2(csrc, gen / csrc.name)
    shutil.copy2(hsrc, gen / hsrc.name)
    build = dst / "tools" / "build.py"
    text = build.read_text()
    anchor = "    refm_objs = []\n"
    injected = "    refm_sources.append('generated/refm_sample_assets.c')\n    refm_objs = []\n"
    if text.count(anchor) != 1:
        raise SystemExit("Felucca overlay build anchor changed")
    build.write_text(text.replace(anchor, injected, 1))
    print("target sample link-size experiment injected; playback remains disabled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
