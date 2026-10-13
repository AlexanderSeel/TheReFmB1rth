#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Harden the physical beta panel against held-note mode changes.

The ReFm tap/hold layer defers modifier-capable primary actions until button
release. Panic therefore belongs at that deferred primary-action boundary, not
at the original Felucca button edge. Machine changes through ALGORITHM/OCT are
also guarded so a held ACID key cannot survive a context change.
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

    # SEQ, ARP, SCL and SAVE are navigation/context actions. They are now
    # deferred until release by apply_keyboard_performance.py. Only panic when
    # the hold was not consumed and the deferred primary action will fire.
    rep(p,
        "        if (!used) refm_felucca_ui_action(action);\n",
        "        if (!used) {\n"
        "            if (action == 3u || action == 4u || action == 7u || action == 8u) refm_felucca_panic();\n"
        "            refm_felucca_ui_action(action);\n"
        "        }\n")

    rep(p,
        "        else refm_felucca_ui_encoder(5u, -1);\n",
        "        else { refm_felucca_panic(); refm_felucca_ui_encoder(5u, -1); }\n")
    rep(p,
        "        else refm_felucca_ui_encoder(5u, 1);\n",
        "        else { refm_felucca_panic(); refm_felucca_ui_encoder(5u, 1); }\n")
    rep(p,
        "    if ((s = panel_enc(EN_ALGO)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_ui_encoder(5u, (int16_t)s); }\n",
        "    if ((s = panel_enc(EN_ALGO)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_panic(); refm_felucca_ui_encoder(5u, (int16_t)s); }\n")

    print("FM-1 beta held-note page/machine transitions hardened for deferred tap/hold input")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
