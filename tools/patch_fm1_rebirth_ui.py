#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the compact ReFm visual layer and physical-hardware beta integration.

The dedicated ReFm renderer owns the normal 240x240 LCD. Legacy Felucca graph
content is deliberately left untouched; injecting the old compatibility step
strip caused a stray horizontal pattern line to bleed across ReFm pages on
physical hardware.
"""
from __future__ import annotations
import argparse, hashlib, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-overlay").is_file():
        raise SystemExit("apply the ReFmB1rth Felucca overlay first")
    graph = root / "firmware" / "src" / "ui_graph.c"

    # Do not patch ui_graph.c with ReFm step lamps. The dedicated RB-240
    # renderer already draws its own sequencer state, and the legacy graph
    # overlay was visible on unrelated physical pages.
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_hardware_beta_fixes.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_keyboard_performance.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_skin.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_beta3_layout.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_beta3_flam.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_focus_indicator.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_beta_input_safety.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_target_beta_rom.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "enable_rom_ref_diagnostics.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_runtime_diagnostics.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_refm_boot_splash.py"), str(root)], check=True, cwd=ROOT)

    h = hashlib.sha256()
    for path in (
        graph,
        root / "firmware" / "src" / "ui_draw.c",
        root / "firmware" / "src" / "seq.c",
        root / "firmware" / "src" / "ui_input.c",
        root / "firmware" / "src" / "main.c",
        root / "firmware" / "src" / "audio.c",
        root / "firmware" / "refm" / "proto" / "acid303.c",
    ):
        h.update(path.read_bytes())
    (root / ".refm-ui-overlay").write_text(
        f"style=compact-rebirth-inspired+refm-hardware-beta+rb240-beta3-approved+909-flam+keyboard-performance+focus-indicator+no-legacy-graph-strip+refm-boot-splash\nsha256={h.hexdigest()}\n"
    )
    print("FM-1 dedicated ReFm hardware UI + RB-240 + keyboard performance/focus + clean graph ownership + boot splash + runtime diagnostics applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
