// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_MIXER_FX_H
#define REFM_MIXER_FX_H

#include <stdint.h>

#define MIX_TRACKS 4u

/* Browser/WASM has cheap linear memory, so give the performance rack a proper
   musical delay range and longer diffusion paths. The embedded FM-1 keeps the
   compact buffers until the physical CPU/RAM profiling gate proves that more
   pool can safely be spent on effects. */
#if defined(__EMSCRIPTEN__)
#define MIX_DELAY_MAX 65535u          /* 1.486 s @ 44.1 kHz */
#define MIX_REVERB_A 1499u            /* 34.0 ms */
#define MIX_REVERB_B 2111u            /* 47.9 ms */
#define MIX_REVERB_FEEDBACK_MAX 32400 /* ~0.989 */
#else
#define MIX_DELAY_MAX 2048u
#define MIX_REVERB_A 353u
#define MIX_REVERB_B 521u
#define MIX_REVERB_FEEDBACK_MAX 31000
#endif

typedef struct {
    int16_t level;
    int16_t pan;
    uint8_t mute;
    uint8_t solo;
    uint8_t delay_send;
    uint8_t reverb_send;
} mixer_track_t;

typedef struct {
    mixer_track_t track[MIX_TRACKS];
    int16_t delay_l[MIX_DELAY_MAX];
    int16_t delay_r[MIX_DELAY_MAX];
    uint16_t delay_pos;
    uint16_t delay_len;
    int16_t delay_feedback;
    int16_t delay_mix;

    /* Stereo cross-feedback reverb. WASM uses much longer mutually-prime-ish
       lines; FM-1 retains the compact version for its current pool budget. */
    int16_t reverb_a[MIX_REVERB_A];
    int16_t reverb_b[MIX_REVERB_B];
    uint16_t reverb_pos_a;
    uint16_t reverb_pos_b;
    int16_t reverb_feedback;
    int16_t reverb_mix;
    int16_t reverb_damp;
    int32_t reverb_lp_a;
    int32_t reverb_lp_b;

    int16_t drive;
    int16_t compressor_threshold;
    int16_t filter_cutoff;
    int32_t filter_l;
    int32_t filter_r;
} mixer_fx_t;

void mixer_fx_init(mixer_fx_t *m);
void mixer_fx_set_delay(mixer_fx_t *m, uint16_t samples, int16_t feedback, int16_t mix);
void mixer_fx_set_reverb(mixer_fx_t *m, int16_t feedback, int16_t mix, int16_t damp);
void mixer_fx_process(mixer_fx_t *m, const int16_t input[MIX_TRACKS], int16_t *out_l, int16_t *out_r);

#endif
