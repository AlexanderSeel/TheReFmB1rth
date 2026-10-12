#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Harden the physical beta panel against held-note mode changes.

The note keyboard is intentionally disabled on STEP/DRUM/SONG/PROJECT pages.
Before a page or machine change repurposes the keys, send ReFm panic so a key
held across that transition cannot leave an ACID gate stuck on.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def rep(path: Path, old: str, new: str) -> None:
    s = path.read_text()
    if s.count(old) != 1:
        raise SystemExit(f"beta input safety anchor mismatch: {old!r}")
    path.write_text(s.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    p = args.felucca.resolve() / "firmware" / "src" / "ui_input.c"

    rep(p,
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\n",
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\nextern void refm_felucca_panic(void);\n")

    for button, action in (("B_SEQ", "3u"), ("B_ARP", "4u"), ("B_SCL", "7u"), ("B_SAVE", "8u")):
        rep(p,
            f"    if (RB_HIT({button}))  refm_felucca_ui_action({action});\n" if button != "B_SAVE" else f"    if (RB_HIT({button})) refm_felucca_ui_action({action});\n",
            f"    if (RB_HIT({button})) {{ refm_felucca_panic(); refm_felucca_ui_action({action}); }}\n")

    rep(p,
        "        else refm_felucca_ui_encoder(5u, -1);\n",
        "        else { refm_felucca_panic(); refm_felucca_ui_encoder(5u, -1); }\n")
    rep(p,
        "        else refm_felucca_ui_encoder(5u, 1);\n",
        "        else { refm_felucca_panic(); refm_felucca_ui_encoder(5u, 1); }\n")
    rep(p,
        "    if ((s = panel_enc(EN_ALGO)) != 0) refm_felucca_ui_encoder(5u, (int16_t)s);\n",
        "    if ((s = panel_enc(EN_ALGO)) != 0) { refm_felucca_panic(); refm_felucca_ui_encoder(5u, (int16_t)s); }\n")

    print("FM-1 beta held-note page/machine transitions hardened")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
