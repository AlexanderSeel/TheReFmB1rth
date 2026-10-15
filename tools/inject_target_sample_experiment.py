#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inject and enable bundled drum samples in the FM-1 target tree.

The workflow builds a baseline target first for an explicit size comparison,
then builds this sample-enabled tree.  This helper turns the latter into the
actual release payload after its own target-memory check passes, avoiding the
old bug where the sample build was measured but the sample-disabled baseline
was still published.
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
    build.write_text(text.replace(anchor, injected, 1))

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

    # Felucca's build.sh ends in an exec. Replace it so a successful sample
    # target build performs its own memory gate, then atomically promotes that
    # exact FWSC/app/loader into build/target and regenerates verification data.
    # The baseline target-memory report is retained for size-delta diagnostics.
    build_sh = dst / "build.sh"
    promote = r'''"$PY" tools/build.py "$@"
ROOT="$(cd ../.. && pwd)"
TARGET="$ROOT/build/target"
mkdir -p "$TARGET"
if [ -f "$TARGET/target-memory.json" ] && [ ! -f "$TARGET/baseline-target-memory.json" ]; then
    cp "$TARGET/target-memory.json" "$TARGET/baseline-target-memory.json"
fi
"$JIELI_TOOLCHAIN/common/bin/objdump" -t build/felucca.elf > "$TARGET/felucca-sample-experiment.syms"
"$PY" "$ROOT/tools/check_target_budget.py" build/felucca.bin "$TARGET/felucca-sample-experiment.syms" --output "$TARGET/sample-experiment-memory.json"
cp "$TARGET/sample-experiment-memory.json" "$TARGET/target-memory.json"
cp build/felucca.fwsc "$TARGET/TheReFmB1rth-experimental.fwsc"
cp build/felucca.bin "$TARGET/TheReFmB1rth-app.bin"
cp build/loader/ota.bin "$TARGET/ota.bin"
cp .refm-overlay "$TARGET/refm-overlay.txt"
cp .refm-ui-overlay "$TARGET/refm-ui-overlay.txt"
"$PY" "$ROOT/tools/release_manifest.py" "$TARGET/TheReFmB1rth-experimental.fwsc" --commit "${GITHUB_SHA:-local}" --output "$TARGET/manifest.json"
"$PY" "$ROOT/tools/release_manifest.py" "$TARGET/TheReFmB1rth-experimental.fwsc" --verify "$TARGET/manifest.json"
(
    cd "$TARGET"
    sha256sum TheReFmB1rth-experimental.fwsc TheReFmB1rth-app.bin ota.bin > SHA256SUMS
    sha256sum -c SHA256SUMS
)
echo "sample-enabled FM-1 target promoted to release payload"
'''
    replace_once(build_sh, 'exec "$PY" tools/build.py "$@"\n', promote)

    print("target bundled samples injected, enabled, memory-gated and configured for release promotion")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
