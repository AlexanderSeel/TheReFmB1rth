// SPDX-License-Identifier: GPL-3.0-only
/* Deterministic acid-only fingerprints for sonic calibration.
   These are characterization probes, not claims of analog equivalence. */
#include <stdint.h>
#include <stdio.h>
#include "../firmware/proto/acid303.h"

static uint64_t fnv16(uint64_t h, int32_t v) {
    uint16_t x = (uint16_t)(int16_t)v;
    h ^= (uint8_t)x; h *= 1099511628211ull;
    h ^= (uint8_t)(x >> 8); h *= 1099511628211ull;
    return h;
}

static void report(const char *name, acid303_t *s, uint32_t frames) {
    uint64_t h = 1469598103934665603ull;
    uint64_t abs_sum = 0u;
    int32_t peak = 0;
    for (uint32_t i = 0; i < frames; ++i) {
        int32_t y = acid303_process(s);
        int32_t a = y < 0 ? -y : y;
        if (a > peak) peak = a;
        abs_sum += (uint32_t)a;
        h = fnv16(h, y);
    }
    printf("acid_ref %-16s hash=%016llx peak=%ld mean_abs=%llu\n",
           name, (unsigned long long)h, (long)peak,
           (unsigned long long)(abs_sum / frames));
}

static acid303_t base_voice(uint8_t square) {
    acid303_t s;
    acid303_init(&s);
    s.square = square;
    s.lfo_amount = 0u;
    s.drive = 0u;
    return s;
}

int main(void) {
    acid303_t s;

    s = base_voice(0u);
    s.cutoff = 22000u; s.resonance = 3000u; s.env_mod = 0u;
    acid303_set_note(&s, 48u, 0u, 0u);
    report("saw_open", &s, 22050u);

    s = base_voice(1u);
    s.cutoff = 22000u; s.resonance = 3000u; s.env_mod = 0u;
    acid303_set_note(&s, 48u, 0u, 0u);
    report("square_open", &s, 22050u);

    s = base_voice(0u);
    s.cutoff = 2500u; s.resonance = 9000u; s.env_mod = 30000u; s.decay = 15000u;
    acid303_set_note(&s, 40u, 0u, 0u);
    report("lowcut_env", &s, 44100u);

    s = base_voice(0u);
    s.cutoff = 15000u; s.resonance = 31500u; s.env_mod = 18000u; s.decay = 10000u;
    acid303_set_note(&s, 45u, 0u, 0u);
    report("high_res", &s, 44100u);

    s = base_voice(0u);
    s.cutoff = 10000u; s.resonance = 25000u; s.env_mod = 27000u;
    s.accent = 30000u;
    acid303_set_note(&s, 43u, 1u, 0u);
    report("accent", &s, 22050u);

    s = base_voice(0u);
    s.cutoff = 12000u; s.resonance = 22000u; s.env_mod = 23000u;
    acid303_set_note(&s, 40u, 0u, 0u);
    for (uint32_t i = 0; i < 5512u; ++i) (void)acid303_process(&s);
    acid303_set_note(&s, 52u, 1u, 1u);
    report("accent_slide", &s, 22050u);

    return 0;
}
