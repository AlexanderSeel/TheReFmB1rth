#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Add conservative runtime stack-watermark diagnostics to pinned Felucca.

This patch is target-only and deliberately avoids replacing Felucca's existing
stack guard. It paints only memory that is safe at early boot, then reports a
high-water estimate through the existing `felucca_dbg` .noinit structure.
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
    new = """    uint32_t prev_stage, prev_page, prev_home, prev_rst, prev_frames;   /* as found at boot */\n    uint32_t ustack_used, sstack_used, ustack_free, sstack_free; /* ReFm runtime watermark */\n} felucca_dbg __attribute__((section(\".noinit\")));\n"""
    replace_once(audio, old, new)


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
 * The JieLi user stack grows downward from _ustack_top.  We cannot repaint the
 * active fm1_cstart frame, so early boot paints from _ustack_lo up to a marker
 * below the current frame.  The unpainted safety band is therefore included in
 * `ustack_used`, making the user-stack result deliberately conservative.
 *
 * The supervisor stack is still unused while interrupts are disabled, so it is
 * painted completely.  Felucca's hardware stack guard remains authoritative;
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


def write_stamp(dst: Path) -> None:
    tracked = [dst / "firmware" / "src" / "audio.c", dst / "firmware" / "src" / "main.c"]
    h = hashlib.sha256()
    for path in tracked:
        h.update(path.relative_to(dst).as_posix().encode())
        h.update(path.read_bytes())
    (dst / STAMP).write_text(
        f"upstream={PIN}\nsha256={h.hexdigest()}\n"
        "fields=ustack_used,sstack_used,ustack_free,sstack_free\n"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("felucca", type=Path, help="pinned Felucca checkout after the ReFm overlay")
    args = parser.parse_args()
    dst = args.felucca.resolve()
    verify_pin(dst)
    patch_audio(dst)
    patch_main(dst)
    write_stamp(dst)
    print("ReFm runtime stack-watermark diagnostics applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
