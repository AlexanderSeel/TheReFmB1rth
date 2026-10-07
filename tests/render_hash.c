// SPDX-License-Identifier: GPL-3.0-only
#include <stdint.h>
#include <stdio.h>
#include "../firmware/proto/groovebox.h"

#define EXPECTED_RENDER_HASH 0xc709644565ca71f7ull

static uint64_t fnv1a_u16(uint64_t h, uint16_t v) {
    h ^= (uint8_t)v; h *= 1099511628211ull;
    h ^= (uint8_t)(v >> 8); h *= 1099511628211ull;
    return h;
}

int main(void) {
    groovebox_t g;
    uint64_t h = 1469598103934665603ull;
    groovebox_init(&g, 128u);
    g.acid_seq[0].step[0].note = 48u;
    g.acid_seq[0].step[0].flags = SEQ16_GATE | SEQ16_ACCENT;
    g.acid_seq[0].step[0].probability = 100u;
    g.acid_seq[0].step[4].note = 51u;
    g.acid_seq[0].step[4].flags = SEQ16_GATE | SEQ16_SLIDE;
    g.acid_seq[0].step[4].probability = 100u;
    g.acid_seq[1].step[2].note = 55u;
    g.acid_seq[1].step[2].flags = SEQ16_GATE;
    g.acid_seq[1].step[2].probability = 100u;
    groovebox_set_drum_step(&g, 0u, 0u, (1u << DRUM_BD) | (1u << DRUM_CH), (1u << DRUM_BD));
    groovebox_set_drum_step(&g, 0u, 4u, (1u << DRUM_SD) | (1u << DRUM_CH), 0u);
    groovebox_set_drum_step(&g, 1u, 2u, (1u << DRUM_CH), 0u);
    groovebox_set_drum_step(&g, 1u, 6u, (1u << DRUM_OH), (1u << DRUM_OH));
    groovebox_advance_step(&g);
    for (uint32_t i = 0; i < 65536u; ++i) {
        int16_t l = 0, r = 0;
        groovebox_process(&g, &l, &r);
        h = fnv1a_u16(h, (uint16_t)l);
        h = fnv1a_u16(h, (uint16_t)r);
    }
    printf("groovebox_render_fnv1a64=%016llx\n", (unsigned long long)h);
    if (h != EXPECTED_RENDER_HASH) {
        fprintf(stderr, "audio fingerprint changed: actual %016llx expected %016llx\n",
                (unsigned long long)h, (unsigned long long)EXPECTED_RENDER_HASH);
        return 1;
    }
    return 0;
}
