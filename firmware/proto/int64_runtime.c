// SPDX-License-Identifier: GPL-3.0-only
#include <stdint.h>

/* The JieLi pi32v2 freestanding link does not provide compiler-rt/libgcc's
   __udivdi3. Current TB-303 fixed-point coefficient code can make clang emit
   that helper after inlining. Keep this target-only and division-free so it
   cannot recurse into itself. */
#ifdef REFM_FELUCCA_PLATFORM
uint64_t __udivdi3(uint64_t numerator, uint64_t denominator) {
    if (!denominator) return UINT64_MAX;
    if (numerator < denominator) return 0u;
    uint64_t quotient = 0u;
    uint64_t bit = 1u;
    while (denominator <= (UINT64_MAX >> 1) && (denominator << 1) <= numerator) {
        denominator <<= 1;
        bit <<= 1;
    }
    while (bit) {
        if (numerator >= denominator) {
            numerator -= denominator;
            quotient |= bit;
        }
        denominator >>= 1;
        bit >>= 1;
    }
    return quotient;
}
#endif
