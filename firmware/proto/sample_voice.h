// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_SAMPLE_VOICE_H
#define REFM_SAMPLE_VOICE_H
#include <stdint.h>

typedef struct {
    const int16_t *pcm;
    uint32_t frames;
    uint32_t cursor_q16;
    uint32_t step_q16;
    uint16_t gain_q15;
    uint8_t active;
} sample_voice_t;

void sample_voice_init(sample_voice_t *v);
void sample_voice_trigger(sample_voice_t *v, const int16_t *pcm, uint32_t frames, uint8_t velocity);
void sample_voice_trigger_rate(sample_voice_t *v, const int16_t *pcm, uint32_t frames, uint8_t velocity, uint32_t step_q16);
int16_t sample_voice_process(sample_voice_t *v);

#endif
