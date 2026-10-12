// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_EDITOR_DIAGNOSTICS_H
#define REFM_EDITOR_DIAGNOSTICS_H

/*
 * Read-only physical runtime diagnostics over Felucca's editor SysEx envelope:
 *   F0 7D 46 4C 91 F7
 *
 * Reply payload after command 91:
 *   version (1 byte), then each field as 5x7-bit little-endian bytes:
 *   magic, halves, last_us, max_us, cpu_q8, late, nested, timer_irqs,
 *   ui_frames, ustack_used, ustack_free, sstack_used, sstack_free.
 *
 * The fields are the same `felucca_dbg` counters consumed by
 * tools/target_runtime_report.py. This command is read-only and does not reset
 * counters, touch flash, or alter transport/audio state.
 */
#define ED_REFM_DIAGNOSTICS 91u

static int refm_diagnostics_handle(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    (void)a;
    if (cmd != ED_REFM_DIAGNOSTICS)
        return 0;
    if (n != 0u) {
        ed_b(0u);              /* malformed request */
        return 1;
    }

    ed_b(1u);                  /* protocol version */
    ed_bk_u32(felucca_dbg.magic);
    ed_bk_u32(felucca_dbg.halves);
    ed_bk_u32(felucca_dbg.last_us);
    ed_bk_u32(felucca_dbg.max_us);
    ed_bk_u32(felucca_dbg.cpu_q8);
    ed_bk_u32(felucca_dbg.late);
    ed_bk_u32(felucca_dbg.nested);
    ed_bk_u32(felucca_dbg.timer_irqs);
    ed_bk_u32(felucca_dbg.ui_frames);
    ed_bk_u32(felucca_dbg.ustack_used);
    ed_bk_u32(felucca_dbg.ustack_free);
    ed_bk_u32(felucca_dbg.sstack_used);
    ed_bk_u32(felucca_dbg.sstack_free);
    return 1;
}

#endif
