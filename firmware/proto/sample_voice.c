// SPDX-License-Identifier: GPL-3.0-only
#include "sample_voice.h"

static int16_t clip16(int32_t x) {
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

void sample_voice_init(sample_voice_t *v) {
    v->pcm = 0;
    v->frames = 0u;
    v->cursor_q16 = 0u;
    v->step_q16 = 65536u;
    v->gain_q15 = 0u;
    v->active = 0u;
}

void sample_voice_trigger_rate(sample_voice_t *v, const int16_t *pcm, uint32_t frames, uint8_t velocity, uint32_t step_q16) {
    v->pcm = pcm;
    v->frames = frames;
    v->cursor_q16 = 0u;
    v->step_q16 = step_q16 ? step_q16 : 65536u;
    v->gain_q15 = (uint16_t)(((uint32_t)velocity * 32767u + 63u) / 127u);
    v->active = (pcm && frames && velocity) ? 1u : 0u;
}

void sample_voice_trigger(sample_voice_t *v, const int16_t *pcm, uint32_t frames, uint8_t velocity) {
    sample_voice_trigger_rate(v, pcm, frames, velocity, 65536u);
}

int16_t sample_voice_process(sample_voice_t *v) {
    uint32_t index, frac;
    int32_t a, b, y;
    if (!v->active || !v->pcm || !v->frames) return 0;
    index = v->cursor_q16 >> 16;
    if (index >= v->frames) {
        v->active = 0u;
        return 0;
    }
    frac = v->cursor_q16 & 0xffffu;
    a = v->pcm[index];
    b = index + 1u < v->frames ? v->pcm[index + 1u] : a;
    y = a + (int32_t)(((int64_t)(b - a) * frac) >> 16);
    y = (int32_t)(((int64_t)y * v->gain_q15) >> 15);
    v->cursor_q16 += v->step_q16;
    if ((v->cursor_q16 >> 16) >= v->frames) v->active = 0u;
    return clip16(y);
}
