#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply TheReFmB1rth to the exact pinned Felucca checkout.

This does not flash hardware. It prepares a build tree by copying the shared
runtime into Felucca, compiling each ReFmB1rth C source as a separate object,
switching the Felucca audio path to the ReFmB1rth renderer, mirroring normalized
USB/TRS MIDI in the application only, wiring ACID controls/graphs into the
physical UI, and exposing Felucca's atomic A/B storage.
"""
from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

PIN = "3dd2b0852bc310a2c1bf00c2d443ac19202c2140"
ROOT = Path(__file__).resolve().parents[1]

REFM_SOURCES = [
    "proto/acid303.c", "proto/int64_runtime.c", "proto/seq16.c", "proto/sample_voice.c", "proto/drum_machine.c", "proto/mixer_fx.c",
    "proto/midi_transport.c", "proto/song.c", "proto/project_store.c", "proto/groovebox.c",
    "proto/midi_router.c", "proto/groovebox_midi.c", "proto/pattern_bank.c",
    "proto/groovebox_pattern.c", "proto/ui_graph_model.c", "integration/refm_target.c",
    "integration/refm_felucca_bridge.c",
]


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"overlay anchor mismatch in {path}: expected exactly one occurrence of {old!r}")
    path.write_text(text.replace(old, new, 1))


def verify_pin(dst: Path) -> None:
    actual = subprocess.check_output(["git", "-C", str(dst), "rev-parse", "HEAD"], text=True).strip()
    if actual != PIN:
        raise SystemExit(f"refusing overlay: Felucca HEAD is {actual}, expected {PIN}")


def generate_runtime_assets() -> None:
    subprocess.run([sys.executable, str(ROOT / "tools" / "generate_303_wavetables.py")], cwd=ROOT, check=True)
    subprocess.run([sys.executable, str(ROOT / "tools" / "generate_303_wavetables.py"), "--check"], cwd=ROOT, check=True)


def copy_runtime(dst: Path) -> None:
    target = dst / "firmware" / "refm"
    if target.exists():
        shutil.rmtree(target)
    (target / "proto").mkdir(parents=True)
    (target / "integration").mkdir(parents=True)
    for src in sorted((ROOT / "firmware" / "proto").glob("*.[ch]")):
        shutil.copy2(src, target / "proto" / src.name)
    for src in sorted((ROOT / "firmware" / "integration").glob("*.[ch]")):
        shutil.copy2(src, target / "integration" / src.name)
    (target / "sources.txt").write_text("\n".join(REFM_SOURCES) + "\n")


def patch_audio(dst: Path) -> None:
    audio = dst / "firmware" / "src" / "audio.c"
    replace_once(audio, "    mix_block(out, n);\n", "    refm_felucca_audio_block(out, n);\n")
    replace_once(audio, "#define HALF_WORDS (HALF_FRAMES * 2u)\n", "#define HALF_WORDS (HALF_FRAMES * 2u)\nextern void refm_felucca_audio_block(int32_t *out, uint32_t frames);\n")


def patch_midi(dst: Path) -> None:
    usb = dst / "firmware" / "src" / "usb.c"
    replace_once(usb, "static int midi_enqueue(uint32_t pkt, uint32_t source)\n{\n", "#ifdef REFM_APP\nextern void refm_felucca_midi_packet(uint32_t packet);\n#define REFM_MIRROR_MIDI(pkt) refm_felucca_midi_packet(pkt)\n#else\n#define REFM_MIRROR_MIDI(pkt) ((void)0)\n#endif\nstatic int midi_enqueue(uint32_t pkt, uint32_t source)\n{\n    REFM_MIRROR_MIDI(pkt);\n")
    felucca = dst / "firmware" / "src" / "felucca.c"
    replace_once(felucca, '#include "usb.c"\n', '#define REFM_APP 1\n#include "usb.c"\n#undef REFM_APP\n')


def patch_storage(dst: Path) -> None:
    felucca = dst / "firmware" / "src" / "felucca.c"
    wrappers = '''#include "storage.c"\n/* ReFmB1rth project slots reuse Felucca's CRC-checked A/B commit protocol. */\nint refm_platform_storage_save(uint32_t slot, const void *src, uint32_t len)\n{\n    if (slot >= 4u) return -1;\n    return st_save(OBJ_PROJECT0 + slot, src, len);\n}\nint refm_platform_storage_load(uint32_t slot, void *dst, uint32_t max)\n{\n    if (slot >= 4u) return -1;\n    return st_load(OBJ_PROJECT0 + slot, dst, max);\n}\n'''
    replace_once(felucca, '#include "storage.c"\n', wrappers)


def patch_ui(dst: Path) -> None:
    ui_input = dst / "firmware" / "src" / "ui_input.c"
    replace_once(ui_input, "    *vp = (int16_t)v;\n    if (pg->scope != SC_GLOBAL) motion_capture(TSEL, (uint32_t)(vp - TSEL->p), *vp);\n", "    *vp = (int16_t)v;\n    if (song.sel < 2u) {\n        uint8_t rk = 0u;\n        if (pg->fam == FAM_ENV && pg->graph == GR_ADSR) rk = 1u;\n        else if (pg->fam == FAM_LFO && pg->graph == GR_LFO) rk = 2u;\n        else if (pg->fam == FAM_LFO && pg->id[slot] == P_LD_FLT) rk = 3u;\n        else if (pg->fam == FAM_EDIT && pg->id[slot] >= P_E0 && pg->id[slot] <= P_E3) rk = 4u;\n        if (rk) refm_felucca_ui_param(song.sel, rk, (uint8_t)slot, *vp);\n    }\n    if (pg->scope != SC_GLOBAL) motion_capture(TSEL, (uint32_t)(vp - TSEL->p), *vp);\n")
    replace_once(ui_input, "static void edit_param(uint32_t slot, int32_t steps)\n{\n", "extern void refm_felucca_ui_param(uint8_t track, uint8_t kind, uint8_t slot, int16_t value);\nstatic void edit_param(uint32_t slot, int32_t steps)\n{\n")

    ui_graph = dst / "firmware" / "src" / "ui_graph.c"
    helper = '''extern int refm_felucca_ui_curve(uint8_t track, uint8_t kind, int16_t *out, uint32_t count);\n\n/* TheReFmB1rth ACID pages: render the same 64-point model used by the browser/WASM UI. */\nstatic int graph_refm(uint8_t kind, uint16_t c)\n{\n    int16_t yv[64];\n    int32_t i, px = 0, py = 0;\n    if (song.sel >= 2u || refm_felucca_ui_curve(song.sel, kind, yv, 64u) != 64) return 0;\n    if (kind == 2u) cv_rect(PANEL_X0, graph_ht / 2, PANEL_W, 1, T_RAISE);\n    else cv_rect(PANEL_X0, 88 * graph_ht / 100, PANEL_W, 1, T_RAISE);\n    for (i = 0; i < 64; ++i) {\n        int32_t x = PANEL_X0 + i * (PANEL_W - 1) / 63;\n        int32_t y;\n        if (kind == 2u) y = graph_ht / 2 - (int32_t)yv[i] * (42 * graph_ht / 100) / 32768;\n        else y = 88 * graph_ht / 100 - (int32_t)yv[i] * (78 * graph_ht / 100) / 32767;\n        if (i) cv_line_t(px, py, x, y, c, 2);\n        px = x; py = y;\n    }\n    return 1;\n}\n\n'''
    replace_once(ui_graph, "static void draw_graph(void)\n{\n", helper + "static void draw_graph(void)\n{\n")
    replace_once(ui_graph, "    } else {\n        switch (pg->graph) {\n", "    } else {\n        if (song.sel < 2u && pg->fam == FAM_ENV && pg->graph == GR_ADSR) { graph_refm(1u, c); }\n        else if (song.sel < 2u && pg->fam == FAM_LFO && pg->graph == GR_LFO) { graph_refm(2u, c); }\n        else if (song.sel < 2u && pg->fam == FAM_EDIT && pg->id[0] == P_E0) { graph_refm(4u, c); }\n        else switch (pg->graph) {\n")


def patch_build(dst: Path) -> None:
    build = dst / "tools" / "build.py"
    text = build.read_text()
    compile_anchor = '    tc_all(("cc", "-c", FW / "crt0.S", "-o", OUT / "crt0.o"),\n'
    if compile_anchor not in text:
        raise SystemExit("build.py compile anchor changed")
    source_literal = repr(REFM_SOURCES)
    injected = (f"    refm_sources = {source_literal}\n" "    refm_objs = []\n" "    for idx, rel in enumerate(refm_sources):\n" "        obj = OUT / f\"refm_{idx:02d}.o\"\n" "        tc(\"cc\", *flags, \"-DREFM_FELUCCA_PLATFORM=1\", \"-Ifirmware/refm/proto\",\n" "           \"-Ifirmware/refm/integration\", \"-c\", FW / \"refm\" / rel, \"-o\", obj)\n" "        refm_objs.append(obj)\n")
    text = text.replace(compile_anchor, injected + compile_anchor, 1)
    link_old = '       OUT / "felucca.o", "-o", elf)\n'
    link_new = '       OUT / "felucca.o", *refm_objs, "-o", elf)\n'
    if text.count(link_old) != 1:
        raise SystemExit("build.py link anchor changed")
    build.write_text(text.replace(link_old, link_new, 1))


def write_stamp(dst: Path) -> None:
    files = [dst / "firmware" / "src" / "audio.c", dst / "firmware" / "src" / "usb.c", dst / "firmware" / "src" / "felucca.c", dst / "firmware" / "src" / "ui_input.c", dst / "firmware" / "src" / "ui_graph.c", dst / "tools" / "build.py", dst / "firmware" / "refm" / "sources.txt"]
    h = hashlib.sha256()
    for path in files:
        h.update(path.relative_to(dst).as_posix().encode())
        h.update(path.read_bytes())
    (dst / ".refm-overlay").write_text(f"upstream={PIN}\nsha256={h.hexdigest()}\n")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path, help="pinned Felucca checkout")
    ap.add_argument("--check", action="store_true", help="apply and validate anchors but do not invoke build")
    args = ap.parse_args()
    dst = args.felucca.resolve()
    verify_pin(dst)
    generate_runtime_assets()
    copy_runtime(dst)
    patch_audio(dst)
    patch_midi(dst)
    patch_storage(dst)
    patch_ui(dst)
    patch_build(dst)
    write_stamp(dst)
    print(f"TheReFmB1rth overlay applied to Felucca {PIN}")
    print("Hardware flashing remains disabled; build/test the resulting tree first.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
