#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inject generated sample assets into an already-applied Felucca overlay.

The target build links the selected PCM assets and enables them in the ReFm
808/909 drum machines. Lanes without bundled samples continue to use the
synthesized fallback, so the target remains a hybrid engine.
"""
from __future__ import annotations
import argparse, shutil
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"sample target anchor mismatch in {path}: expected exactly one occurrence")
    path.write_text(text.replace(old, new, 1))


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
        raise SystemExit("refusing sample target injection: ReFmB1rth overlay stamp is missing")

    gen = dst / "firmware" / "refm" / "generated"
    gen.mkdir(parents=True, exist_ok=True)
    shutil.copy2(csrc, gen / csrc.name)
    shutil.copy2(hsrc, gen / hsrc.name)

    # Compile the generated const PCM unit into XIP.
    build = dst / "tools" / "build.py"
    text = build.read_text()
    anchor = "    refm_objs = []\n"
    injected = "    refm_sources.append('generated/refm_sample_assets.c')\n    refm_objs = []\n"
    if text.count(anchor) != 1:
        raise SystemExit("Felucca overlay build anchor changed")
    text = text.replace(anchor, injected, 1)
    build.write_text(text)

    # Enable the generated sample table only in this target tree. The normal
    # host/WASM build keeps its existing explicit sample controls.
    groovebox = dst / "firmware" / "refm" / "proto" / "groovebox.c"
    replace_once(
        groovebox,
        '#include "groovebox.h"\n#include <string.h>\n',
        '#include "groovebox.h"\n#include "../generated/refm_sample_assets.h"\n#include <string.h>\n',
    )
    old_init = (
        'drum_machine_init(&g->drum[0],DRUM_MODEL_808);'
        'drum_machine_init(&g->drum[1],DRUM_MODEL_909);'
    )
    new_init = (
        'drum_machine_init(&g->drum[0],DRUM_MODEL_808);'
        'drum_machine_init(&g->drum[1],DRUM_MODEL_909);'
        'refm_attach_bundled_samples(&g->drum[0],DRUM_MODEL_808);'
        'refm_attach_bundled_samples(&g->drum[1],DRUM_MODEL_909);'
        'drum_machine_enable_samples(&g->drum[0],1u);'
        'drum_machine_enable_samples(&g->drum[1],1u);'
    )
    replace_once(groovebox, old_init, new_init)

    print("target bundled samples injected and playback enabled; unbundled lanes use synthesis fallback")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
