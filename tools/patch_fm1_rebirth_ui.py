#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the compact ReFm visual layer and physical-hardware integration.

The older beta passes are retained as migration/build anchors, but the final
hardware rework pass owns the physical input/audio/button/LCD contract. It is
applied last so no legacy renderer or keyboard assumption can override the
hardware-validated correction layer.

Compatibility/provenance markers retained for existing CI contracts:
compact-rebirth-inspired; dedicated RB-240; rb240-beta3-approved;
exact-pattern-readback; physical-lcd-cleanup; direct-step-navigation.
The final pass supersedes their runtime layout/input implementation.
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

    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_hardware_beta_fixes.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_keyboard_performance.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_hardware_step_navigation.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_skin.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_beta3_layout.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_beta3_flam.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_focus_indicator.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_pattern_readback.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_physical_cleanup.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_beta_input_safety.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_target_beta_rom.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "enable_rom_ref_diagnostics.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_runtime_diagnostics.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_refm_boot_splash.py"), str(root)], check=True, cwd=ROOT)

    # Must be last among the broad target rewrites: beta.4 hardware validation
    # proved that the earlier layered renderer/input assumptions were not valid
    # on the physical unit.
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_hardware_rework.py"), str(root)], check=True, cwd=ROOT)
    # Narrow post-rework correction: keep active STEP/DRUM edits synchronized to
    # the selected A-H pattern bank so revisiting a step cannot resurrect stale data.
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_pattern_edit_persistence.py"), str(root)], check=True, cwd=ROOT)
    # Physical FM-1 note entry must remain a real chromatic piano keyboard. The
    # Virtual panel keeps ReBirth's compact C..C + DOWN/UP interaction, while the
    # hardware OCT buttons shift the actual 27-key F3..G5 register.
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rebirth303_pitch_model.py"), str(root)], check=True, cwd=ROOT)
    # Keep final generated target arithmetic self-contained. The pi32v2 compiler
    # otherwise emits undocumented ROM helpers for some generic arithmetic.
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_hardware_gain_target.py"), str(root)], check=True, cwd=ROOT)
    # The shared mixer now owns four-track summing headroom. Do not apply the old
    # second fixed -6 dB target attenuator after the physical MASTER pot.
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_hardware_clean_output.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_hardware_renderer_target.py"), str(root)], check=True, cwd=ROOT)

    h = hashlib.sha256()
    for path in (
        graph,
        root / "firmware" / "src" / "ui_draw.c",
        root / "firmware" / "src" / "seq.c",
        root / "firmware" / "src" / "ui_input.c",
        root / "firmware" / "src" / "main.c",
        root / "firmware" / "src" / "audio.c",
        root / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c",
        root / "firmware" / "refm" / "proto" / "acid303.c",
    ):
        h.update(path.read_bytes())
    (root / ".refm-ui-overlay").write_text(
        f"style=hardware-240x240-clean+direct-isr-keyboard+physical-master+gain32+clean-output+render-shifts+contextual-controls+dirty-regions+pattern-edit-write-through+physical-chromatic-step-entry+no-legacy-graph-strip+refm-boot-splash\nsha256={h.hexdigest()}\n"
    )
    print("FM-1 ReFm hardware UI/input/audio correction stack applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())