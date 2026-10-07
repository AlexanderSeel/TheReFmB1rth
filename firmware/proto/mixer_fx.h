// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_MIXER_FX_H
#define REFM_MIXER_FX_H

#include <stdint.h>

#define MIX_TRACKS 4u
#define MIX_DELAY_MAX 2048u

typedef struct {
    int16_t level;
    int16_t pan;
    uint8_t mute;
    uint8_t solo;
    uint8_t delay_send;
} mixer_track_t;

typedef struct {
    mixer_track_t track[MIX_TRACKS];
    int16_t delay_l[MIX_DELAY_MAX];
    int16_t delay_r[MIX_DELAY_MAX];
    uint16_t delay_pos;
    uint16_t delay_len;
    int16_t delay_feedback;
    int16_t delay_mix;
    int16_t drive;
    int16_t compressor_threshold;
    int16_t filter_cutoff;
    int32_t filter_l;
    int32_t filter_r;
} mixer_fx_t;

void mixer_fx_init(mixer_fx_t *m);
void mixer_fx_set_delay(mixer_fx_t *m, uint16_t samples, int16_t feedback, int16_t mix);
void mixer_fx_process(mixer_fx_t *m, const int16_t input[MIX_TRACKS], int16_t *out_l, int16_t *out_r);

#endif
