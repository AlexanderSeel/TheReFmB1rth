#!/usr/bin/env python3
"""Verify that the pinned Felucca checkout still exposes the integration anchors we rely on.

This is intentionally read-only. It does not patch or build the upstream tree; it prevents us
from silently drifting onto an incompatible Felucca layout before the JieLi target build exists.
"""
from __future__ import annotations

import pathlib
import subprocess
import sys

PIN = "3dd2b0852bc310a2c1bf00c2d443ac19202c2140"

REQUIRED = {
    "firmware/src/felucca.c": (
        '#include "fx.c"',
        '#include "seq.c"',
        '#include "audio.c"',
        '#include "project.c"',
        '#include "main.c"',
    ),
    "firmware/src/audio.c": (
        "static void audio_block(int32_t *out, uint32_t n)",
        "mix_block(out, n);",
        "uac_tap(out, n);",
        "fm1_alnk0_irq",
    ),
    "tools/build.py": (
        'FW / "src" / "felucca.c"',
        'OUT / "felucca.o"',
        'FW / "app.ld"',
    ),
}


def head(repo: pathlib.Path) -> str:
    return subprocess.check_output(["git", "-C", str(repo), "rev-parse", "HEAD"], text=True).strip()


def main() -> int:
    repo = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".upstream/felucca")
    if not (repo / ".git").exists():
        print(f"ERROR: {repo} is not a Felucca git checkout", file=sys.stderr)
        return 2
    actual = head(repo)
    if actual != PIN:
        print(f"ERROR: Felucca pin mismatch: expected {PIN}, got {actual}", file=sys.stderr)
        return 3
    failed = False
    for rel, anchors in REQUIRED.items():
        path = repo / rel
        if not path.is_file():
            print(f"ERROR: missing upstream integration file: {rel}", file=sys.stderr)
            failed = True
            continue
        text = path.read_text(encoding="utf-8")
        for anchor in anchors:
            if anchor not in text:
                print(f"ERROR: upstream anchor changed in {rel}: {anchor!r}", file=sys.stderr)
                failed = True
    if failed:
        return 4
    print(f"Felucca integration anchors verified at {PIN}")
    print("audio: Q15 interleaved stereo mix_block -> UAC/DAC path present")
    print("build: monolithic felucca.c target + app linker path present")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
