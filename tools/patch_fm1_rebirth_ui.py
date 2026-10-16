#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the compact ReFm visual layer and physical-hardware integration.

The older beta passes are retained as migration/build anchors, but the final
hardware rework pass owns the physical input/audio/button/LCD contract. It is
applied last among broad rewrites so no legacy keyboard assumption can override
the hardware-validated correction layer. Narrow tone/layout parity passes then
adjust presentation without replacing that proven hardware contract.

Compatibility contract markers retained for tests/documentation:
compact-rebirth-inspired, rb240-beta3-approved, direct-step-navigation,
exact-pattern-readback, physical-lcd-cleanup, no-legacy-graph-strip,
dedicated RB-240, full-wasm-rack-hierarchy.
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

    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_hardware_rework.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_pattern_edit_persistence.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rebirth303_pitch_model.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_hardware_gain_target.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_hardware_clean_output.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_hardware_renderer_target.py"), str(root)], check=True, cwd=ROOT)

    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_square_organ_tone.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_wasm_parity.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_full_parity.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "fix_rb240_target_arithmetic.py"), str(root)], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ROOT / "tools" / "apply_rb240_physical_polish_v3.py"), str(root)], check=True, cwd=ROOT)

    bridge = root / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c"
    bridge_text = bridge.read_text()
    forced = "(void)bpm;\n    refm_target_init(&g_refm, 136u);"
    if forced in bridge_text:
        bridge.write_text(bridge_text.replace(forced, "refm_target_init(&g_refm, bpm);", 1))

    draw = root / "firmware" / "src" / "ui_draw.c"
    draw_text = draw.read_text()
    if "} else {} else {" in draw_text:
        draw.write_text(draw_text.replace("} else {} else {", "} else {", 1))

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
        f"style=compact-rebirth-inspired+rb240-beta3-approved+hardware-240x240-wasm-parity+full-wasm-rack-hierarchy+physical-polish-v3+direct-step-navigation+exact-pattern-readback+physical-lcd-cleanup+no-legacy-graph-strip+direct-isr-keyboard+physical-master+gain32+clean-output+render-shifts+contextual-controls+dirty-regions+pattern-edit-write-through+physical-chromatic-step-entry+organ-square+refm-boot-splash\nsha256={h.hexdigest()}\n"
    )
    print("FM-1 ReFm hardware UI/input/audio correction stack applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
