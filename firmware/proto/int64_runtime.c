// SPDX-License-Identifier: GPL-3.0-only
#include <stdint.h>
#include <limits.h>

/* The JieLi pi32v2 freestanding link omits a small subset of compiler-rt/libgcc
   helpers. Keep local, division-free implementations here so target DSP can use
   ordinary C without jumping to missing runtime symbols. */
#ifdef REFM_FELUCCA_PLATFORM
uint64_t __udivdi3(uint64_t numerator, uint64_t denominator) {
    uint64_t quotient = 0u;
    uint64_t bit = 1u;
    if (!denominator) return UINT64_MAX;
    if (numerator < denominator) return 0u;
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

typedef union {
    uint32_t u;
    float f;
} refm_sf_bits_t;

/* IEEE-754 binary32 conversion helpers. They deliberately use only integer
   shifts/comparisons plus bit reinterpretation, so compiling these functions
   cannot recursively request the helpers they implement. Exact round-to-nearest
   is unnecessary for the control/phase uses here; truncating the discarded low
   integer bits is deterministic and well below the 303 control resolution. */
static float refm_u32_to_float_bits(uint32_t value) {
    refm_sf_bits_t out;
    uint32_t msb = 31u;
    uint32_t mantissa;
    uint32_t shift;

    if (!value) {
        out.u = 0u;
        return out.f;
    }
    while (((value >> msb) & 1u) == 0u) --msb;
    if (msb <= 23u) {
        mantissa = (value << (23u - msb)) & 0x007fffffu;
    } else {
        shift = msb - 23u;
        mantissa = (value >> shift) & 0x007fffffu;
    }
    out.u = ((msb + 127u) << 23) | mantissa;
    return out.f;
}

float __floatunsisf(uint32_t value) {
    return refm_u32_to_float_bits(value);
}

float __floatsisf(int32_t value) {
    uint32_t magnitude;
    float result;
    if (value >= 0) return refm_u32_to_float_bits((uint32_t)value);
    magnitude = (~(uint32_t)value) + 1u;
    result = refm_u32_to_float_bits(magnitude);
    return -result;
}

int32_t __fixsfsi(float value) {
    refm_sf_bits_t in;
    uint32_t sign, exponent, mantissa, magnitude;
    int32_t e;

    in.f = value;
    sign = in.u >> 31;
    exponent = (in.u >> 23) & 0xffu;
    if (!exponent) return 0;
    e = (int32_t)exponent - 127;
    if (e < 0) return 0;
    if (e > 30) return sign ? INT32_MIN : INT32_MAX;
    mantissa = (in.u & 0x007fffffu) | 0x00800000u;
    magnitude = e >= 23 ? mantissa << (uint32_t)(e - 23) : mantissa >> (uint32_t)(23 - e);
    if (sign) {
        if (magnitude >= 0x80000000u) return INT32_MIN;
        return -(int32_t)magnitude;
    }
    if (magnitude > 0x7fffffffu) return INT32_MAX;
    return (int32_t)magnitude;
}

uint32_t __fixunssfsi(float value) {
    refm_sf_bits_t in;
    uint32_t exponent, mantissa;
    int32_t e;

    in.f = value;
    if (in.u >> 31) return 0u;
    exponent = (in.u >> 23) & 0xffu;
    if (!exponent) return 0u;
    e = (int32_t)exponent - 127;
    if (e < 0) return 0u;
    if (e > 31) return UINT32_MAX;
    mantissa = (in.u & 0x007fffffu) | 0x00800000u;
    return e >= 23 ? mantissa << (uint32_t)(e - 23) : mantissa >> (uint32_t)(23 - e);
}
#endif
