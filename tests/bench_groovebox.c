// SPDX-License-Identifier: GPL-3.0-only
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "../firmware/proto/groovebox.h"

/* Mirrors the target lifetime: the real FM-1 runtime lives in .pool, not on
   the tiny call stack. Keeping the stress instance static also makes host CI
   exercise the same long-lived object model. */
static groovebox_t g_stress;

static void fill_worst_case(groovebox_t *g) {
    const uint16_t all = (uint16_t)((1u << DRUM_VOICES) - 1u);
    for (unsigned a = 0; a < 2u; ++a) {
        g->acid[a].cutoff = a ? 18000u : 11000u;
        g->acid[a].resonance = 32000u;
        g->acid[a].env_mod = 30000u;
        g->acid[a].decay = 25000u;
        g->acid[a].accent = 30000u;
        g->acid[a].drive = 12000u;
        for (unsigned s = 0; s < 16u; ++s) {
            seq16_step_t *st = &g->acid_seq[a].step[s];
            st->note = (uint8_t)(36u + ((s * 5u + a * 7u) % 24u));
            st->flags = SEQ16_GATE | SEQ16_ACCENT | (s ? SEQ16_SLIDE : 0u);
            st->probability = 255u;
            st->micro = 0;
        }
    }
    for (unsigned d = 0; d < 2u; ++d)
        for (unsigned s = 0; s < 16u; ++s)
            groovebox_set_drum_step(g, (uint8_t)d, (uint8_t)s, all, all);

    for (unsigned t = 0; t < MIX_TRACKS; ++t) {
        g->mixer.track[t].level = 32767;
        g->mixer.track[t].pan = 0;
        g->mixer.track[t].delay_send = 255u;
    }
    g->mixer.drive = 14000;
    g->mixer.compressor_threshold = 9000;
    g->mixer.filter_cutoff = 26000;
    g->mixer.delay_len = 2205u;
    g->mixer.delay_feedback = 25000;
    g->mixer.delay_mix = 18000;
    groovebox_advance_step(g);
}

int main(void) {
    enum { FRAMES = 1000000 };
    uint32_t checksum = 0u;
    int16_t l = 0, r = 0;
    groovebox_init(&g_stress, 150u);
    fill_worst_case(&g_stress);

    clock_t begin = clock();
    for (uint32_t i = 0; i < FRAMES; ++i) {
        groovebox_process(&g_stress, &l, &r);
        checksum = checksum * 33u + (uint16_t)l;
        checksum = checksum * 33u + (uint16_t)r;
    }
    clock_t end = clock();
    double seconds = (double)(end - begin) / (double)CLOCKS_PER_SEC;
    double fps = seconds > 0.0 ? (double)FRAMES / seconds : 0.0;
    double realtime = fps / 44100.0;
    printf("groovebox_stress frames=%d seconds=%.6f frames_per_s=%.0f realtime_x=%.2f checksum=%08x\n",
           FRAMES, seconds, fps, realtime, checksum);
    return 0;
}
