#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Add conservative runtime and ReFm physical-path diagnostics to pinned Felucca.

The patch keeps Felucca's existing stack guard, adds conservative stack watermarks,
and exposes read-only counters through editor SysEx command 91. Protocol v2 adds
ReFm-specific evidence so a hardware silence report can be localized to input,
transport, renderer, or the later DAC/UAC path without changing working DSP.
"""
from __future__ import annotations

import argparse
import hashlib
import subprocess
from pathlib import Path

PIN = "3dd2b0852bc310a2c1bf00c2d443ac19202c2140"
STAMP = ".refm-runtime-diag"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(
            f"runtime diagnostic anchor mismatch in {path}: expected exactly one occurrence of {old!r}"
        )
    path.write_text(text.replace(old, new, 1))


def verify_pin(dst: Path) -> None:
    actual = subprocess.check_output(
        ["git", "-C", str(dst), "rev-parse", "HEAD"], text=True
    ).strip()
    if actual != PIN:
        raise SystemExit(f"refusing runtime diagnostic patch: Felucca HEAD is {actual}, expected {PIN}")


def patch_audio(dst: Path) -> None:
    audio = dst / "firmware" / "src" / "audio.c"
    old = """    uint32_t prev_stage, prev_page, prev_home, prev_rst, prev_frames;   /* as found at boot */\n} felucca_dbg __attribute__((section(\".noinit\")));\n"""
    new = """    uint32_t prev_stage, prev_page, prev_home, prev_rst, prev_frames;   /* as found at boot */\n    uint32_t ustack_used, sstack_used, ustack_free, sstack_free; /* ReFm runtime watermark */\n    uint32_t refm_blocks, refm_nonzero_blocks; /* physical ReFm renderer evidence */\n} felucca_dbg __attribute__((section(\".noinit\")));\n"""
    replace_once(audio, old, new)

    # apply_felucca_overlay.py has already replaced Felucca mix_block() at this
    # point. Count every target render and whether that Q15 block contains any
    # non-zero sample before Felucca scales it to the 24-bit DAC domain.
    replace_once(
        audio,
        "    refm_felucca_audio_block(out, n);\n"
        "    refm_felucca_emit_state_readback_if_pending();\n",
        "    refm_felucca_audio_block(out, n);\n"
        "    felucca_dbg.refm_blocks++;\n"
        "    {\n"
        "        uint32_t refm_i;\n"
        "        for (refm_i = 0u; refm_i < n * 2u; ++refm_i)\n"
        "            if (out[refm_i] != 0) { felucca_dbg.refm_nonzero_blocks++; break; }\n"
        "    }\n"
        "    refm_felucca_emit_state_readback_if_pending();\n",
    )


def patch_bridge(dst: Path) -> None:
    bridge = dst / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c"
    replace_once(
        bridge,
        "static uint8_t g_focus_active;\nstatic uint8_t g_focus_region;\n",
        "static uint8_t g_focus_active;\nstatic uint8_t g_focus_region;\n"
        "static volatile uint32_t g_diag_key_edges;\n"
        "static volatile uint32_t g_diag_key_consumed;\n"
        "static volatile uint32_t g_diag_live_note_edges;\n"
        "static volatile uint32_t g_diag_transport_events;\n"
        "static volatile uint32_t g_diag_transport_playing;\n",
    )
    replace_once(
        bridge,
        "    g_focus_active = 0u;\n    g_focus_region = UI240_FOCUS_ACID;\n}\n\nvoid refm_felucca_audio_block",
        "    g_focus_active = 0u;\n    g_focus_region = UI240_FOCUS_ACID;\n"
        "    g_diag_key_edges = 0u;\n    g_diag_key_consumed = 0u;\n"
        "    g_diag_live_note_edges = 0u;\n    g_diag_transport_events = 0u;\n"
        "    g_diag_transport_playing = 0u;\n}\n\nvoid refm_felucca_audio_block",
    )
    replace_once(
        bridge,
        "    ensure_ready();\n    r = ui240_performance_key(&g_ui, &g_refm.groovebox, g_modifier, key, note, on);\n"
        "    if (r.drum_trigger)\n",
        "    ensure_ready();\n    g_diag_key_edges++;\n"
        "    r = ui240_performance_key(&g_ui, &g_refm.groovebox, g_modifier, key, note, on);\n"
        "    if (r.drum_trigger)\n",
    )
    replace_once(
        bridge,
        "    if (r.consumed) return;\n"
        "    if (g_ui.machine >= UI240_MACHINE_808 || note > 127u) return;\n",
        "    if (r.consumed) { g_diag_key_consumed++; return; }\n"
        "    if (g_ui.machine >= UI240_MACHINE_808 || note > 127u) return;\n"
        "    g_diag_live_note_edges++;\n",
    )
    replace_once(
        bridge,
        "    g_panel_playing = playing;\n"
        "    refm_target_set_external_clock(&g_refm, 0u);\n",
        "    g_panel_playing = playing;\n"
        "    g_diag_transport_events++;\n    g_diag_transport_playing = playing;\n"
        "    refm_target_set_external_clock(&g_refm, 0u);\n",
    )

    getter = r'''
uint32_t refm_felucca_diag_value(uint8_t id)
{
    ensure_ready();
    switch (id) {
    case 0u: return g_diag_key_edges;
    case 1u: return g_diag_key_consumed;
    case 2u: return g_diag_live_note_edges;
    case 3u: return g_diag_transport_events;
    case 4u: return g_diag_transport_playing;
    default: return 0u;
    }
}

'''
    anchor = "uint16_t refm_felucca_ui_value(uint8_t id)\n{\n"
    replace_once(bridge, anchor, getter + anchor)


def patch_main(dst: Path) -> None:
    main = dst / "firmware" / "src" / "main.c"
    replace_once(
        main,
        "extern uint32_t _pool_start[], _pool_end[], _rt_start[], _rt_end[], _rt_load[];\n",
        "extern uint32_t _pool_start[], _pool_end[], _rt_start[], _rt_end[], _rt_load[];\n"
        "extern char _ustack_top[];\n",
    )

    helper_anchor = "/* power-on: the parts with their default sounds (TRK_DEF); the sequencers empty */\n"
    helper = r'''/* ReFm beta runtime stack diagnostics.
 *
 * The JieLi user stack grows downward from _ustack_top. We cannot repaint the
 * active fm1_cstart frame, so early boot paints from _ustack_lo up to a marker
 * below the current frame. The unpainted safety band is therefore included in
 * `ustack_used`, making the user-stack result deliberately conservative.
 *
 * The supervisor stack is still unused while interrupts are disabled, so it is
 * painted completely. Felucca's hardware stack guard remains authoritative;
 * this watermark only measures headroom during a successful run.
 */
#define REFM_STACK_FILL 0xA55A3CC3u
#define REFM_USTACK_BOOT_MARGIN 256u
static uint32_t *refm_ustack_fill_end;

static void refm_stack_watermark_init(void)
{
    volatile uint32_t marker = 0u;
    uint32_t *p;
    uintptr_t here = (uintptr_t)&marker;
    uintptr_t lo = (uintptr_t)_ustack_lo;
    uintptr_t hi = (uintptr_t)_ustack_top;
    uintptr_t stop = here > REFM_USTACK_BOOT_MARGIN ? here - REFM_USTACK_BOOT_MARGIN : lo;

    if (stop < lo) stop = lo;
    if (stop > hi) stop = hi;
    stop &= ~(uintptr_t)3u;
    refm_ustack_fill_end = (uint32_t *)stop;

    for (p = (uint32_t *)(void *)_ustack_lo; p < refm_ustack_fill_end; ++p)
        *p = REFM_STACK_FILL;
    for (p = (uint32_t *)(void *)_sstack_lo; p < (uint32_t *)(void *)_sstack_top; ++p)
        *p = REFM_STACK_FILL;
}

static uint32_t refm_stack_first_dirty(uint32_t *lo, uint32_t *hi)
{
    uint32_t *p = lo;
    while (p < hi && *p == REFM_STACK_FILL)
        ++p;
    return (uint32_t)(uintptr_t)p;
}

static void refm_stack_watermark_update(void)
{
    uint32_t u_dirty = refm_stack_first_dirty((uint32_t *)(void *)_ustack_lo, refm_ustack_fill_end);
    uint32_t s_dirty = refm_stack_first_dirty((uint32_t *)(void *)_sstack_lo, (uint32_t *)(void *)_sstack_top);
    uint32_t u_lo = (uint32_t)(uintptr_t)_ustack_lo;
    uint32_t u_top = (uint32_t)(uintptr_t)_ustack_top;
    uint32_t s_lo = (uint32_t)(uintptr_t)_sstack_lo;
    uint32_t s_top = (uint32_t)(uintptr_t)_sstack_top;

    felucca_dbg.ustack_used = u_top - u_dirty;
    felucca_dbg.ustack_free = u_dirty - u_lo;
    felucca_dbg.sstack_used = s_top - s_dirty;
    felucca_dbg.sstack_free = s_dirty - s_lo;
}

'''
    replace_once(main, helper_anchor, helper + helper_anchor)

    replace_once(
        main,
        "    felucca_dbg.max_us = 0;\n",
        "    felucca_dbg.max_us = 0;\n"
        "    refm_stack_watermark_update();\n",
    )

    loop_anchor = "        fm1_wdt_feed();\n"
    loop_patch = """        fm1_wdt_feed();\n        {\n            static uint32_t refm_stack_ms;\n            if ((uint32_t)(fm1_ms - refm_stack_ms) >= 1000u) {\n                refm_stack_ms = fm1_ms;\n                refm_stack_watermark_update();\n            }\n        }\n"""
    replace_once(main, loop_anchor, loop_patch)

    init_anchor = """    for (s = _rt_load, d = _rt_start; d < _rt_end; s++, d++)\n        *d = *s;                                /* flash driver code that must run from RAM */\n    fm1_mailbox_clear();\n"""
    init_patch = """    for (s = _rt_load, d = _rt_start; d < _rt_end; s++, d++)\n        *d = *s;                                /* flash driver code that must run from RAM */\n    refm_stack_watermark_init();\n    fm1_mailbox_clear();\n"""
    replace_once(main, init_anchor, init_patch)


def patch_editor(dst: Path) -> None:
    editor = dst / "firmware" / "src" / "editor.c"
    replace_once(
        editor,
        '#include "../refm/integration/refm_editor_backup.h"\n',
        '#include "../refm/integration/refm_editor_backup.h"\n'
        '#include "../refm/integration/refm_editor_diagnostics.h"\n',
    )
    replace_once(
        editor,
        '    if (refm_backup_handle(cmd, a, na)) { ed_send(); return; }\n',
        '    if (refm_backup_handle(cmd, a, na)) { ed_send(); return; }\n'
        '    if (refm_diagnostics_handle(cmd, a, na)) { ed_send(); return; }\n',
    )

    diag = dst / "firmware" / "refm" / "integration" / "refm_editor_diagnostics.h"
    text = diag.read_text()
    text = text.replace(
        " *   version (1 byte), then each field as 5x7-bit little-endian bytes:\n"
        " *   magic, halves, last_us, max_us, cpu_q8, late, nested, timer_irqs,\n"
        " *   ui_frames, ustack_used, ustack_free, sstack_used, sstack_free.\n",
        " *   version (1 byte), then each field as 5x7-bit little-endian bytes.\n"
        " *   v2 keeps all v1 runtime fields and appends:\n"
        " *   refm_blocks, refm_nonzero_blocks, key_edges, key_consumed,\n"
        " *   live_note_edges, transport_events, transport_playing.\n",
    )
    text = text.replace(
        "#define ED_REFM_DIAGNOSTICS 91u\n",
        "#define ED_REFM_DIAGNOSTICS 91u\n"
        "extern uint32_t refm_felucca_diag_value(uint8_t id);\n",
    )
    text = text.replace("    ed_b(1u);                  /* protocol version */\n", "    ed_b(2u);                  /* protocol version */\n")
    text = text.replace(
        "    ed_bk_u32(felucca_dbg.sstack_free);\n    return 1;\n",
        "    ed_bk_u32(felucca_dbg.sstack_free);\n"
        "    ed_bk_u32(felucca_dbg.refm_blocks);\n"
        "    ed_bk_u32(felucca_dbg.refm_nonzero_blocks);\n"
        "    ed_bk_u32(refm_felucca_diag_value(0u)); /* key edges */\n"
        "    ed_bk_u32(refm_felucca_diag_value(1u)); /* consumed */\n"
        "    ed_bk_u32(refm_felucca_diag_value(2u)); /* live note edges */\n"
        "    ed_bk_u32(refm_felucca_diag_value(3u)); /* transport events */\n"
        "    ed_bk_u32(refm_felucca_diag_value(4u)); /* transport playing */\n"
        "    return 1;\n",
    )
    diag.write_text(text)


def write_stamp(dst: Path) -> None:
    tracked = [
        dst / "firmware" / "src" / "audio.c",
        dst / "firmware" / "src" / "main.c",
        dst / "firmware" / "src" / "editor.c",
        dst / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c",
        dst / "firmware" / "refm" / "integration" / "refm_editor_diagnostics.h",
    ]
    h = hashlib.sha256()
    for path in tracked:
        h.update(path.relative_to(dst).as_posix().encode())
        h.update(path.read_bytes())
    (dst / STAMP).write_text(
        f"upstream={PIN}\nsha256={h.hexdigest()}\n"
        "protocol=2\n"
        "fields=halves,last_us,max_us,cpu_q8,late,nested,timer_irqs,ui_frames,"
        "ustack_used,ustack_free,sstack_used,sstack_free,refm_blocks,"
        "refm_nonzero_blocks,key_edges,key_consumed,live_note_edges,"
        "transport_events,transport_playing\n"
        "editor_command=91\n"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("felucca", type=Path, help="pinned Felucca checkout after the ReFm overlay")
    args = parser.parse_args()
    dst = args.felucca.resolve()
    verify_pin(dst)
    patch_audio(dst)
    patch_bridge(dst)
    patch_main(dst)
    patch_editor(dst)
    write_stamp(dst)
    print("ReFm runtime stack + physical-path diagnostics protocol v2 applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
