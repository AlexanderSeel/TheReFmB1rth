#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""First physical FM-1 beta fixes discovered on hardware.

This patch intentionally applies only to the generated/pinned Felucca build tree.
It connects the FM-1 panel keyboard and PLAY state to the ReFm runtime, removes
misleading ADSR/LFO hooks, and adds a silent-voice fast path to reduce target CPU.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"hardware-beta anchor mismatch in {path}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-overlay").is_file():
        raise SystemExit("apply ReFm overlay before hardware beta fixes")

    # The physical keyboard and PLAY button belong to Felucca's proven scanner,
    # but the audible engine is ReFm. Mirror changed note keys and transport state
    # into ReFm at the existing keyboard control-rate hook.
    seq = root / "firmware" / "src" / "seq.c"
    replace_once(
        seq,
        "static void keyboard_block(void)\n{\n    uint32_t cur = fm1_in.notes, ch, k;\n",
        "static uint32_t refm_prev_panel_notes;\n"
        "static uint8_t refm_prev_panel_playing;\n"
        "extern void refm_felucca_key_note(uint8_t track, uint8_t note, uint8_t on);\n"
        "extern void refm_felucca_panel_transport(uint8_t playing);\n"
        "static void keyboard_block(void)\n{\n"
        "    uint32_t cur = fm1_in.notes, ch, k;\n"
        "    uint32_t refm_changed = cur ^ refm_prev_panel_notes;\n"
        "    if ((uint8_t)(song.playing != 0u) != refm_prev_panel_playing) {\n"
        "        refm_prev_panel_playing = song.playing ? 1u : 0u;\n"
        "        refm_felucca_panel_transport(refm_prev_panel_playing);\n"
        "    }\n"
        "    if (refm_changed) {\n"
        "        for (k = 0; k < 27u; ++k) {\n"
        "            uint32_t bit = 1u << k;\n"
        "            if (refm_changed & bit) {\n"
        "                uint32_t note = kb_map(TSEL, k);\n"
        "                uint8_t on = (cur & bit) ? 1u : 0u;\n"
        "                /* NOTE ON only in normal keyboard mode; NOTE OFF always\n"
        "                   propagates so changing pages cannot leave a stuck note. */\n"
        "                if (note <= 127u && (!on || song.grid == 0u))\n"
        "                    refm_felucca_key_note((uint8_t)song.sel, (uint8_t)note, on);\n"
        "            }\n"
        "        }\n"
        "        refm_prev_panel_notes = cur;\n"
        "    }\n"
    )

    # The first hardware test showed that exposing Felucca ENV/LFO pages as ReFm
    # controls is confusing and wrong for a 303 workflow. Keep only EDIT E0..E3
    # as a temporary 4-knob bridge until the dedicated ReFm pages replace it.
    ui = root / "firmware" / "src" / "ui_input.c"
    replace_once(
        ui,
        "        if (pg->fam == FAM_ENV && pg->graph == GR_ADSR) rk = 1u;\n"
        "        else if (pg->fam == FAM_LFO && pg->graph == GR_LFO) rk = 2u;\n"
        "        else if (pg->fam == FAM_LFO && pg->id[slot] == P_LD_FLT) rk = 3u;\n"
        "        else if (pg->fam == FAM_EDIT && pg->id[slot] >= P_E0 && pg->id[slot] <= P_E3) rk = 4u;\n",
        "        if (pg->fam == FAM_EDIT && pg->id[slot] >= P_E0 && pg->id[slot] <= P_E3) rk = 4u;\n"
    )

    # Remove the fake ReFm ADSR/LFO graph substitution. Those were still Felucca
    # pages with ReFm curves painted over them, which is why the promised screens
    # did not actually exist on hardware.
    graph = root / "firmware" / "src" / "ui_graph.c"
    replace_once(
        graph,
        "        if (song.sel < 2u && pg->fam == FAM_ENV && pg->graph == GR_ADSR) { graph_refm(1u, c); }\n"
        "        else if (song.sel < 2u && pg->fam == FAM_LFO && pg->graph == GR_LFO) { graph_refm(2u, c); }\n"
        "        else if (song.sel < 2u && pg->fam == FAM_EDIT && pg->id[0] == P_E0) { graph_refm(4u, c); }\n"
        "        else switch (pg->graph) {\n",
        "        if (song.sel < 2u && pg->fam == FAM_EDIT && pg->id[0] == P_E0) { graph_refm(4u, c); }\n"
        "        else switch (pg->graph) {\n"
    )

    # Silent ACID voices were still evaluating envelope, coefficient mapping,
    # two oscillator/filter substeps and post filters every sample. On the FM-1
    # that is wasted CPU. idle is set only after the release has fully decayed.
    acid = root / "firmware" / "refm" / "proto" / "acid303.c"
    replace_once(
        acid,
        "int32_t acid303_process(acid303_t *s) {\n    slide_step(s);\n",
        "int32_t acid303_process(acid303_t *s) {\n"
        "#ifdef REFM_FELUCCA_PLATFORM\n"
        "    if (s->idle && !s->gate && !s->amp) return 0;\n"
        "#endif\n"
        "    slide_step(s);\n"
    )

    (root / ".refm-hardware-beta").write_text(
        "panel_transport=mirrored\n"
        "panel_keyboard=acid_a_b\n"
        "legacy_adsr_lfo=disabled\n"
        "acid_idle_fastpath=enabled\n"
    )
    print("FM-1 hardware beta fixes applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
