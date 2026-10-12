// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_EDITOR_BACKUP_H
#define REFM_EDITOR_BACKUP_H

#include <stddef.h>

/*
 * ReFm project transfer over Felucca's existing editor SysEx envelope:
 *   F0 7D 46 4C 90 op ... F7
 *
 * Command 90 is outside the pinned Felucca 1.0.5 editor command ranges
 * (notably FM6 uses 68..71 and MENU uses 72..73).
 *
 * The implementation deliberately reuses editor_backup.c's 3840-byte staging
 * area, 7-bit packer/unpacker and zlib-compatible CRC32.  The transferred bytes
 * are the normal groovebox_save() blob, so the existing ReFm project header,
 * version and CRC are checked again by groovebox_load() on restore.
 *
 * op 0 capture:  request [0] -> [0, rc, len(5x7), crc(5x7)]
 * op 1 get:      [1, off(5x7), count(2x7)] -> [1, rc, off, count, pack7 data]
 * op 2 begin:    [2, len(5x7), crc(5x7)] -> [2, rc]
 * op 3 data:     [3, off(5x7), pack7 data] -> [3, rc, next-off(5x7)]
 * op 4 commit:   [4] -> [4, rc]
 * op 5 abort:    [5] -> [5, 0]
 *
 * rc 0 success, 1 malformed/range, 2 CRC/project validation, 3 transport
 * running, 5 staging generation changed.
 */
#define ED_REFM_PROJECT 90u

extern int refm_felucca_save(uint8_t *out, size_t cap, size_t *written);
extern int refm_felucca_load(const uint8_t *blob, size_t len);
extern uint16_t refm_felucca_ui_value(uint8_t id);

static uint8_t refm_bk_valid, refm_bk_put;
static uint32_t refm_bk_len, refm_bk_crc, refm_bk_pos, refm_bk_gen;

static uint32_t refm_bk_capture(void)
{
    size_t written = 0;
    if (refm_felucca_ui_value(3u))
        return 3u;                                      /* deterministic snapshot: stop first */
    ed_bk_valid = 0; ed_bk_put = 0;
    refm_bk_gen = ++proj_wire_gen;                      /* own the shared staging area */
    if (refm_felucca_save(ED_BK_RAW, ED_BK_MAX, &written) || !written || written > ED_BK_MAX)
        return 2u;
    refm_bk_len = (uint32_t)written;
    refm_bk_crc = st_crc32(ED_BK_RAW, refm_bk_len);
    refm_bk_valid = 1u;
    refm_bk_put = 0u;
    return 0u;
}

static uint32_t refm_bk_begin(const uint8_t *a, uint32_t n)
{
    uint32_t len, crc;
    if (n != 11u || refm_felucca_ui_value(3u))
        return n == 11u ? 3u : 1u;
    if (a[5] > 15u || a[10] > 15u)
        return 1u;
    len = ed_bk_r32(a + 1u);
    crc = ed_bk_r32(a + 6u);
    if (!len || len > ED_BK_MAX)
        return 1u;
    ed_bk_valid = 0; ed_bk_put = 0;
    refm_bk_gen = ++proj_wire_gen;
    refm_bk_len = len;
    refm_bk_crc = crc;
    refm_bk_pos = 0u;
    refm_bk_valid = 0u;
    refm_bk_put = 1u;
    return 0u;
}

static uint32_t refm_bk_data(const uint8_t *a, uint32_t n)
{
    uint32_t off, count;
    if (!refm_bk_put || refm_bk_gen != proj_wire_gen)
        return 5u;
    if (n < 8u || a[5] > 15u)
        return 1u;
    off = ed_bk_r32(a + 1u);
    if (off != refm_bk_pos || off >= refm_bk_len)
        return 1u;
    count = ed_unpack7(a + 6u, n - 6u, ed_smp_buf, 256u);
    if (!count || count > refm_bk_len - refm_bk_pos)
        return 1u;
    memcpy(ED_BK_RAW + refm_bk_pos, ed_smp_buf, count);
    refm_bk_pos += count;
    return 0u;
}

static uint32_t refm_bk_commit(void)
{
    int rc;
    if (!refm_bk_put || refm_bk_gen != proj_wire_gen)
        return 5u;
    refm_bk_put = 0u;
    if (refm_felucca_ui_value(3u))
        return 3u;
    if (refm_bk_pos != refm_bk_len || st_crc32(ED_BK_RAW, refm_bk_len) != refm_bk_crc)
        return 2u;
    rc = refm_felucca_load(ED_BK_RAW, refm_bk_len);
    if (rc)
        return 2u;                                      /* project codec/version/CRC rejected it */
    refm_bk_valid = 0u;
    return 0u;
}

static int refm_backup_handle(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    uint32_t op, rc = 0u;
    if (cmd != ED_REFM_PROJECT)
        return 0;
    if (!n) {
        ed_b(127u); ed_b(1u);
        return 1;
    }
    op = a[0];
    ed_b(op);
    if (op == 0u) {
        rc = n == 1u ? refm_bk_capture() : 1u;
        ed_b(rc); ed_bk_u32(rc ? 0u : refm_bk_len); ed_bk_u32(rc ? 0u : refm_bk_crc);
        return 1;
    }
    if (op == 1u) {
        uint32_t off = n >= 6u && a[5] <= 15u ? ed_bk_r32(a + 1u) : 0u;
        uint32_t count = n == 8u ? (uint32_t)a[6] | (uint32_t)a[7] << 7 : 0u;
        rc = !refm_bk_valid || refm_bk_gen != proj_wire_gen ? 5u :
             n != 8u || a[5] > 15u || !count || count > 256u || off > refm_bk_len || count > refm_bk_len - off ? 1u : 0u;
        ed_b(rc); ed_bk_u32(off); ed_b(rc ? 0u : count & 127u); ed_b(rc ? 0u : count >> 7);
        if (!rc) ed_bk_pack(ED_BK_RAW + off, count);
        return 1;
    }
    if (op == 2u) {
        rc = refm_bk_begin(a, n); ed_b(rc); return 1;
    }
    if (op == 3u) {
        rc = refm_bk_data(a, n); ed_b(rc); ed_bk_u32(refm_bk_pos); return 1;
    }
    if (op == 4u) {
        rc = n == 1u ? refm_bk_commit() : 1u; ed_b(rc); return 1;
    }
    if (op == 5u) {
        refm_bk_put = refm_bk_valid = 0u; ++proj_wire_gen; ed_b(n == 1u ? 0u : 1u); return 1;
    }
    ed_b(1u);
    return 1;
}

#endif
