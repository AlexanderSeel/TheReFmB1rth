// SPDX-License-Identifier: GPL-3.0-only
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "../firmware/proto/acid303.h"

int main(void) {
    enum { FRAMES = 2000000 };
    acid303_t s;
    uint32_t checksum = 0u;
    acid303_init(&s);
    s.cutoff = 9000u;
    s.resonance = 27000u;
    s.env_mod = 23000u;
    s.decay = 16000u;
    s.accent = 25000u;
    acid303_set_note(&s, 48u, 1u, 0u);
    clock_t begin = clock();
    for (uint32_t i = 0; i < FRAMES; ++i) {
        if (i == 500000u) acid303_set_note(&s, 55u, 0u, 1u);
        if (i == 1000000u) { s.square = 1u; acid303_set_note(&s, 43u, 1u, 1u); }
        if (i == 1500000u) acid303_set_note(&s, 60u, 0u, 1u);
        checksum = checksum * 33u + (uint16_t)acid303_process(&s);
    }
    clock_t end = clock();
    double seconds = (double)(end - begin) / (double)CLOCKS_PER_SEC;
    double mframes = seconds > 0.0 ? ((double)FRAMES / seconds) / 1000000.0 : 0.0;
    printf("acid_bench frames=%d seconds=%.6f Mframes_per_s=%.3f checksum=%08x\n",
           FRAMES, seconds, mframes, checksum);
    return 0;
}
