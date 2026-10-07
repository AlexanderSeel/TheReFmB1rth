#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply TheReFmB1rth to the exact pinned Felucca checkout.

This does not flash hardware. It prepares a build tree by copying the shared
runtime into Felucca, compiling each ReFmB1rth C source as a separate object,
switching the Felucca audio path to the ReFmB1rth renderer, mirroring normalized
USB/TRS MIDI, and exposing Felucca's proven atomic A/B project storage.
"""
from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
from pathlib import Path

PIN = "3dd2b0852bc310a2c1bf00c2d443ac19202c2140"
ROOT = Path(__file__).resolve().parents[1]

REFM_SOURCES = [
    "proto/acid303.c", "proto/seq16.c", "proto/drum_machine.c", "proto/mixer_fx.c",
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
    replace_once(
        audio,
        "#define HALF_WORDS (HALF_FRAMES * 2u)\n",
        "#define HALF_WORDS (HALF_FRAMES * 2u)\nextern void refm_felucca_audio_block(int32_t *out, uint32_t frames);\n",
    )


def patch_midi(dst: Path) -> None:
    usb = dst / "firmware" / "src" / "usb.c"
    replace_once(
        usb,
        "static int midi_enqueue(uint32_t pkt, uint32_t source)\n{\n",
        "extern void refm_felucca_midi_packet(uint32_t packet);\n"
        "static int midi_enqueue(uint32_t pkt, uint32_t source)\n{\n"
        "    refm_felucca_midi_packet(pkt);\n",
    )


def patch_storage(dst: Path) -> None:
    felucca = dst / "firmware" / "src" / "felucca.c"
    wrappers = '''#include "storage.c"\n/* ReFmB1rth project slots reuse Felucca's CRC-checked A/B commit protocol. */\nint refm_platform_storage_save(uint32_t slot, const void *src, uint32_t len)\n{\n    if (slot >= 4u) return -1;\n    return st_save(OBJ_PROJECT0 + slot, src, len);\n}\nint refm_platform_storage_load(uint32_t slot, void *dst, uint32_t max)\n{\n    if (slot >= 4u) return -1;\n    return st_load(OBJ_PROJECT0 + slot, dst, max);\n}\n'''
    replace_once(felucca, '#include "storage.c"\n', wrappers)


def patch_build(dst: Path) -> None:
    build = dst / "tools" / "build.py"
    text = build.read_text()
    compile_anchor = '    tc_all(("cc", "-c", FW / "crt0.S", "-o", OUT / "crt0.o"),\n'
    if compile_anchor not in text:
        raise SystemExit("build.py compile anchor changed")
    source_literal = repr(REFM_SOURCES)
    injected = (
        f"    refm_sources = {source_literal}\n"
        "    refm_objs = []\n"
        "    for idx, rel in enumerate(refm_sources):\n"
        "        obj = OUT / f\"refm_{idx:02d}.o\"\n"
        "        tc(\"cc\", *flags, \"-DREFM_FELUCCA_PLATFORM=1\", \"-Ifirmware/refm/proto\",\n"
        "           \"-Ifirmware/refm/integration\", \"-c\", FW / \"refm\" / rel, \"-o\", obj)\n"
        "        refm_objs.append(obj)\n"
    )
    text = text.replace(compile_anchor, injected + compile_anchor, 1)
    link_old = '       OUT / "felucca.o", "-o", elf)\n'
    link_new = '       OUT / "felucca.o", *refm_objs, "-o", elf)\n'
    if text.count(link_old) != 1:
        raise SystemExit("build.py link anchor changed")
    build.write_text(text.replace(link_old, link_new, 1))


def write_stamp(dst: Path) -> None:
    files = [
        dst / "firmware" / "src" / "audio.c",
        dst / "firmware" / "src" / "usb.c",
        dst / "firmware" / "src" / "felucca.c",
        dst / "tools" / "build.py",
        dst / "firmware" / "refm" / "sources.txt",
    ]
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
    copy_runtime(dst)
    patch_audio(dst)
    patch_midi(dst)
    patch_storage(dst)
    patch_build(dst)
    write_stamp(dst)
    print(f"TheReFmB1rth overlay applied to Felucca {PIN}")
    print("Hardware flashing remains disabled; build/test the resulting tree first.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
