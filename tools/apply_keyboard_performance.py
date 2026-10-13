#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Patch the physical FM-1 beta overlay with keyboard-first ReFm interaction.

Runs after apply_hardware_beta_fixes.py. It deliberately uses Felucca only for
physical key/button scanning; interpretation lives in ui240_performance.h and
refm_felucca_bridge.c so WASM and hardware share the same musical behavior.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"keyboard-performance anchor mismatch in {path}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-beta").is_file():
        raise SystemExit("apply hardware beta fixes before keyboard performance overlay")

    seq = root / "firmware" / "src" / "seq.c"
    replace_once(
        seq,
        "extern void refm_felucca_key_note(uint8_t track, uint8_t note, uint8_t on);\n"
        "extern int refm_felucca_keys_are_editor(void);\n",
        "extern void refm_felucca_key_event(uint8_t key, uint8_t note, uint8_t on);\n",
    )
    replace_once(
        seq,
        "                if (note <= 127u && (!on || (song.grid == 0u && !refm_felucca_keys_are_editor())))\n"
        "                    refm_felucca_key_note((uint8_t)song.sel, (uint8_t)note, on);\n",
        "                if (note <= 127u)\n"
        "                    refm_felucca_key_event((uint8_t)k, (uint8_t)note, on);\n",
    )

    ui = root / "firmware" / "src" / "ui_input.c"
    replace_once(
        ui,
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\n\n"
        "static int refm_beta_input(uint32_t pressed)\n"
        "{\n"
        "    int32_t s;\n"
        "    uint16_t page = refm_felucca_ui_value(0u);\n"
        "#define RB_HIT(lbl) ((pressed & (1u << panel.btn[(lbl)])) != 0u)\n",
        "extern uint16_t refm_felucca_ui_value(uint8_t id);\n"
        "extern void refm_felucca_ui_modifier(uint8_t modifier);\n"
        "extern void refm_felucca_focus_exit(void);\n\n"
        "static uint32_t refm_mod_mask;\n"
        "static uint8_t refm_mod_action;\n"
        "static uint8_t refm_mod_used;\n\n"
        "static int refm_beta_input(uint32_t pressed)\n"
        "{\n"
        "    int32_t s;\n"
        "    uint8_t layer = 0u;\n"
        "    uint16_t page = refm_felucca_ui_value(0u);\n"
        "#define RB_HIT(lbl) ((pressed & (1u << panel.btn[(lbl)])) != 0u)\n"
        "#define RB_HELD(lbl) ((fm1_in.buttons & (1u << panel.btn[(lbl)])) != 0u)\n"
        "#define RB_BEGIN_MOD(lbl, act) do { \\\n"
        "    if (!refm_mod_mask && RB_HIT(lbl)) { \\\n"
        "        refm_mod_mask = 1u << panel.btn[(lbl)]; \\\n"
        "        refm_mod_action = (act); \\\n"
        "        refm_mod_used = 0u; \\\n"
        "    } \\\n"
        "} while (0)\n"
        "    /* Held layers are intentionally ordered from the most explicit\n"
        "       performance gesture to the broadest page shortcut. SCL/SEL is\n"
        "       the quick FOCUS navigator because its short press remains SONG. */\n"
        "    if (RB_HELD(B_SCL)) layer = 8u;\n"
        "    else if (RB_HELD(B_REC)) layer = 7u;\n"
        "    else if (RB_HELD(B_EDIT)) layer = 6u;\n"
        "    else if (RB_HELD(B_SAVE)) layer = 5u;\n"
        "    else if (RB_HELD(B_FX)) layer = 4u;\n"
        "    else if (RB_HELD(B_GLO)) layer = 3u;\n"
        "    else if (RB_HELD(B_ARP)) layer = 2u;\n"
        "    else if (RB_HELD(B_SEQ)) layer = 1u;\n"
        "    refm_felucca_ui_modifier(layer);\n",
    )
    replace_once(
        ui,
        "    if (RB_HIT(B_HOME)) refm_felucca_ui_action(0u);\n"
        "    if (RB_HIT(B_ENV))  refm_felucca_ui_action(1u);\n"
        "    if (RB_HIT(B_LFO))  refm_felucca_ui_action(2u);\n"
        "    if (RB_HIT(B_SEQ))  refm_felucca_ui_action(3u);\n"
        "    if (RB_HIT(B_ARP))  refm_felucca_ui_action(4u);\n"
        "    if (RB_HIT(B_GLO))  refm_felucca_ui_action(5u);\n"
        "    if (RB_HIT(B_FX))   refm_felucca_ui_action(6u);\n"
        "    if (RB_HIT(B_SCL))  refm_felucca_ui_action(7u);\n"
        "    if (RB_HIT(B_SAVE)) refm_felucca_ui_action(8u);\n"
        "    if (RB_HIT(B_PLAY)) refm_felucca_ui_action(9u);\n"
        "    if (RB_HIT(B_EDIT)) refm_felucca_ui_action(10u);\n"
        "    if (RB_HIT(B_REC))  refm_felucca_ui_action(11u);\n",
        "    if (RB_HIT(B_HOME)) refm_felucca_ui_action(0u);\n"
        "    if (RB_HIT(B_ENV))  refm_felucca_ui_action(1u);\n"
        "    if (RB_HIT(B_LFO))  refm_felucca_ui_action(2u);\n"
        "    if (RB_HIT(B_PLAY)) refm_felucca_ui_action(9u);\n"
        "    RB_BEGIN_MOD(B_SEQ, 3u);\n"
        "    RB_BEGIN_MOD(B_ARP, 4u);\n"
        "    RB_BEGIN_MOD(B_GLO, 5u);\n"
        "    RB_BEGIN_MOD(B_FX, 6u);\n"
        "    RB_BEGIN_MOD(B_SCL, 7u);\n"
        "    RB_BEGIN_MOD(B_SAVE, 8u);\n"
        "    RB_BEGIN_MOD(B_EDIT, 10u);\n"
        "    RB_BEGIN_MOD(B_REC, 11u);\n"
        "    if (refm_mod_mask && fm1_in.notes) refm_mod_used = 1u;\n"
        "    if (refm_mod_mask && !(fm1_in.buttons & refm_mod_mask)) {\n"
        "        uint8_t action = refm_mod_action;\n"
        "        uint8_t used = refm_mod_used;\n"
        "        refm_mod_mask = 0u; refm_mod_action = 0u; refm_mod_used = 0u;\n"
        "        refm_felucca_ui_modifier(0u);\n"
        "        if (!used) { if (action == 7u) refm_felucca_focus_exit(); refm_felucca_ui_action(action); }\n"
        "    }\n",
    )
    replace_once(
        ui,
        "    if ((s = panel_enc(EN_K1)) != 0) refm_felucca_ui_encoder(0u, (int16_t)s);\n"
        "    if ((s = panel_enc(EN_K2)) != 0) refm_felucca_ui_encoder(1u, (int16_t)s);\n"
        "    if ((s = panel_enc(EN_K3)) != 0) refm_felucca_ui_encoder(2u, (int16_t)s);\n"
        "    if ((s = panel_enc(EN_K4)) != 0) refm_felucca_ui_encoder(3u, (int16_t)s);\n"
        "    if ((s = panel_enc(EN_PRESET)) != 0) refm_felucca_ui_encoder(4u, (int16_t)s);\n"
        "    if ((s = panel_enc(EN_ALGO)) != 0) refm_felucca_ui_encoder(5u, (int16_t)s);\n"
        "    if ((s = panel_enc(EN_SELECT)) != 0) refm_felucca_ui_encoder(6u, (int16_t)s);\n",
        "    if ((s = panel_enc(EN_K1)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_ui_encoder(0u, (int16_t)s); }\n"
        "    if ((s = panel_enc(EN_K2)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_ui_encoder(1u, (int16_t)s); }\n"
        "    if ((s = panel_enc(EN_K3)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_ui_encoder(2u, (int16_t)s); }\n"
        "    if ((s = panel_enc(EN_K4)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_ui_encoder(3u, (int16_t)s); }\n"
        "    if ((s = panel_enc(EN_PRESET)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_ui_encoder(4u, (int16_t)s); }\n"
        "    if ((s = panel_enc(EN_ALGO)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_ui_encoder(5u, (int16_t)s); }\n"
        "    if ((s = panel_enc(EN_SELECT)) != 0) { if (refm_mod_mask) refm_mod_used = 1u; refm_felucca_ui_encoder(6u, (int16_t)s); }\n",
    )
    replace_once(ui, "#undef RB_HIT\n", "#undef RB_BEGIN_MOD\n#undef RB_HELD\n#undef RB_HIT\n")

    marker = root / ".refm-keyboard-performance"
    marker.write_text(
        "keyboard=physical-index+note\n"
        "layers=seq,drum,mixer,fx,pattern,edit,live,focus\n"
        "focus=hold-scl\n"
        "tap-hold=deferred-primary-until-release\n"
        "focus-tap=temporary-focus-cleared-before-song\n"
        "modifier-consume=keyboard-or-encoder\n"
        "parity=ui240-performance\n"
    )
    print("FM-1 tap/hold keyboard performance/modifier/focus overlay applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
