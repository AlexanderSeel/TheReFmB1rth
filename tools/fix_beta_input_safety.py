#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Harden immediate physical page/machine transitions against held notes.

Hardware uses direct page-first button actions. Any transition that can change
keyboard interpretation or machine ownership first panics the ReFm live voices,
so a held ACID note cannot survive a SEQ/DRUM/SONG/PROJECT/machine change.
"""
from __future__ import annotations

import argparse
from pathlib import Path


def rep(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"beta input safety anchor mismatch: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("felucca", type=Path)
    args = parser.parse_args()
    p = args.felucca.resolve() / "firmware" / "src" / "ui_input.c"

    rep(
        p,
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\n",
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\nextern void refm_felucca_panic(void);\n",
    )

    for old, new in (
        ("    if (RB_HIT(B_SEQ))  refm_felucca_ui_action(3u);\n",
         "    if (RB_HIT(B_SEQ))  { refm_felucca_panic(); refm_felucca_ui_action(3u); }\n"),
        ("    if (RB_HIT(B_ARP))  refm_felucca_ui_action(4u);\n",
         "    if (RB_HIT(B_ARP))  { refm_felucca_panic(); refm_felucca_ui_action(4u); }\n"),
        ("    if (RB_HIT(B_SCL))  refm_felucca_ui_action(7u);\n",
         "    if (RB_HIT(B_SCL))  { refm_felucca_panic(); refm_felucca_ui_action(7u); }\n"),
        ("    if (RB_HIT(B_SAVE)) refm_felucca_ui_action(8u);\n",
         "    if (RB_HIT(B_SAVE)) { refm_felucca_panic(); refm_felucca_ui_action(8u); }\n"),
        ("        else refm_felucca_ui_encoder(5u, -1);\n",
         "        else { refm_felucca_panic(); refm_felucca_ui_encoder(5u, -1); }\n"),
        ("        else refm_felucca_ui_encoder(5u, 1);\n",
         "        else { refm_felucca_panic(); refm_felucca_ui_encoder(5u, 1); }\n"),
        ("    if ((s = panel_enc(EN_ALGO)) != 0) refm_felucca_ui_encoder(5u, (int16_t)s);\n",
         "    if ((s = panel_enc(EN_ALGO)) != 0) { refm_felucca_panic(); refm_felucca_ui_encoder(5u, (int16_t)s); }\n"),
    ):
        rep(p, old, new)

    print("FM-1 immediate page/machine transitions hardened against held notes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
