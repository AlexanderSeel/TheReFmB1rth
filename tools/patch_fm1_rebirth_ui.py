#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Add a compact ReBirth-inspired step-light strip to the FM-1 ACID graph pages.

This patch is intentionally small: it preserves Felucca's 240x240 card layout and
only adds the visual rhythm of the classic dual-bassline workflow. No original
ReBirth artwork or assets are copied.
"""
from __future__ import annotations
import argparse, hashlib
from pathlib import Path


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
    new = """    /* Compact dual-bassline visual language: 16 grouped step lamps at the\n       bottom of each ACID graph. The active transport step uses ACCENT; inactive\n       lamps use RAISE. This intentionally reuses Felucca primitives only. */\n    {\n        extern uint8_t refm_felucca_step(void);\n        uint32_t st = refm_felucca_step() & 15u;\n        for (i = 0; i < 16; ++i) {\n            int32_t sx = 11 + i * 13 + (i / 4) * 4;\n            uint16_t sc = ((uint32_t)i == st) ? T_ACCENT : T_RAISE;\n            cv_rect(sx, graph_ht - 7, 9, 4, sc);\n        }\n    }\n    return 1;\n}\n\nstatic void draw_graph(void)\n"""
    replace_once(graph, old, new)
    digest = hashlib.sha256(graph.read_bytes()).hexdigest()
    (root / ".refm-ui-overlay").write_text(f"style=compact-rebirth-inspired\nsha256={digest}\n")
    print("FM-1 compact ReBirth-inspired ACID step strip applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
