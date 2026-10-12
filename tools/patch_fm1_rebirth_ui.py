#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Add the compact ReFm visual overlay and first physical-hardware fixes.

The step-light strip remains a temporary Felucca-based visual while the real
ReFm 240x240 pages are implemented. Hardware beta fixes are applied after this
visual patch so every target/release build gets the keyboard/transport bridge,
legacy mapping cleanup and silent-voice CPU fast path.
"""
from __future__ import annotations
import argparse, hashlib, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"FM-1 UI patch anchor mismatch in {path}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-overlay").is_file():
        raise SystemExit("apply the ReFmB1rth Felucca overlay first")
    graph = root / "firmware" / "src" / "ui_graph.c"
    old = """    return 1;\n}\n\nstatic void draw_graph(void)\n"""
    new = """    /* Temporary 16-step transport strip. Dedicated ReFm pages replace\n       Felucca's page hierarchy in the next hardware UI milestone. */\n    {\n        extern uint8_t refm_felucca_step(void);\n        uint32_t st = refm_felucca_step() & 15u;\n        for (i = 0; i < 16; ++i) {\n            int32_t sx = 11 + i * 13 + (i / 4) * 4;\n            uint16_t sc = ((uint32_t)i == st) ? T_ACCENT : T_RAISE;\n            cv_rect(sx, graph_ht - 7, 9, 4, sc);\n        }\n    }\n    return 1;\n}\n\nstatic void draw_graph(void)\n"""
    replace_once(graph, old, new)

    subprocess.run(
        [sys.executable, str(ROOT / "tools" / "apply_hardware_beta_fixes.py"), str(root)],
        check=True,
        cwd=ROOT,
    )

    h = hashlib.sha256()
    for path in (
        graph,
        root / "firmware" / "src" / "seq.c",
        root / "firmware" / "src" / "ui_input.c",
        root / "firmware" / "refm" / "proto" / "acid303.c",
    ):
        h.update(path.read_bytes())
    (root / ".refm-ui-overlay").write_text(
        f"style=hardware-beta-temporary\nsha256={h.hexdigest()}\n"
    )
    print("FM-1 temporary UI + hardware beta fixes applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
