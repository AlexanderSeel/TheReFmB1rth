// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_FREESTANDING_STRING_H
#define REFM_FREESTANDING_STRING_H

/*
 * The JieLi pi32v2 firmware build is freestanding and does not ship the
 * standard C <string.h>.  Felucca has its own libc inside its monolithic
 * application unit, but TheReFmB1rth is intentionally compiled as separate
 * objects.  Keep the tiny operations used by the shared DSP/runtime inline so
 * host, WASM and FM-1 builds do not gain a target libc dependency.
 *
 * This file only shadows <string.h> in the Felucca overlay because that build
 * adds firmware/refm/proto to its include path. Normal host/WASM builds keep
 * using their platform's standard header.
 */
#include <stddef.h>

static inline void *memset(void *dst, int value, size_t count)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned char v = (unsigned char)value;
    size_t i;
    for (i = 0; i < count; ++i)
        d[i] = v;
    return dst;
}

static inline void *memcpy(void *dst, const void *src, size_t count)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    size_t i;
    for (i = 0; i < count; ++i)
        d[i] = s[i];
    return dst;
}

static inline int memcmp(const void *lhs, const void *rhs, size_t count)
{
    const unsigned char *a = (const unsigned char *)lhs;
    const unsigned char *b = (const unsigned char *)rhs;
    size_t i;
    for (i = 0; i < count; ++i) {
        if (a[i] != b[i])
            return (int)a[i] - (int)b[i];
    }
    return 0;
}

#endif
