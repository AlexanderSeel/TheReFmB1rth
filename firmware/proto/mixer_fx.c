// SPDX-License-Identifier: GPL-3.0-only
#include "mixer_fx.h"
#include <string.h>

static int32_t clamp32(int64_t v, int32_t lo, int32_t hi) { return v<lo?lo:v>hi?hi:(int32_t)v; }
static int16_t clip16(int32_t v) { return (int16_t)clamp32(v,-32768,32767); }

void mixer_fx_init(mixer_fx_t *m) {
    memset(m, 0, sizeof(*m));
    for (unsigned i=0; i<MIX_TRACKS; ++i) {
        m->track[i].level = 32767;
        m->track[i].pan = 0;
    }
    m->delay_len = 1102u;
    m->delay_feedback = 14000;
    m->delay_mix = 7000;
    m->drive = 3000;
    m->compressor_threshold = 24576;
    m->filter_cutoff = 30000;
}

void mixer_fx_set_delay(mixer_fx_t *m, uint16_t samples, int16_t feedback, int16_t mix) {
    if (samples == 0u) samples = 1u;
    if (samples > MIX_DELAY_MAX) samples = MIX_DELAY_MAX;
    m->delay_len = samples;
    m->delay_feedback = (int16_t)clamp32(feedback, 0, 30000);
    m->delay_mix = (int16_t)clamp32(mix, 0, 32767);
    if (m->delay_pos >= samples) m->delay_pos = 0u;
}

static int32_t saturate(int32_t x, int16_t drive) {
    int64_t y = ((int64_t)x * (32768 + drive)) >> 15;
    int32_t v = clamp32(y, -65536, 65535);
    int32_t a = v < 0 ? -v : v;
    if (a > 24576) {
        int32_t shaped = 24576 + (a - 24576) / 4;
        v = v < 0 ? -shaped : shaped;
    }
    return clamp32(v, -32768, 32767);
}

static int32_t compress(int32_t x, int16_t threshold) {
    int32_t a = x < 0 ? -x : x;
    if (a <= threshold) return x;
    int32_t v = threshold + (a - threshold) / 4;
    return x < 0 ? -v : v;
}

void mixer_fx_process(mixer_fx_t *m, const int16_t input[MIX_TRACKS], int16_t *out_l, int16_t *out_r) {
    int any_solo = 0;
    for (unsigned i=0; i<MIX_TRACKS; ++i) if (m->track[i].solo) any_solo = 1;

    int64_t left = 0, right = 0, send_l = 0, send_r = 0;
    for (unsigned i=0; i<MIX_TRACKS; ++i) {
        const mixer_track_t *t = &m->track[i];
        if (t->mute || (any_solo && !t->solo)) continue;
        int32_t s = (int32_t)(((int64_t)input[i] * t->level) >> 15);
        int32_t lg = t->pan > 0 ? 32767 - t->pan : 32767;
        int32_t rg = t->pan < 0 ? 32767 + t->pan : 32767;
        int32_t l = (int32_t)(((int64_t)s * lg) >> 15);
        int32_t r = (int32_t)(((int64_t)s * rg) >> 15);
        left += l;
        right += r;
        send_l += ((int64_t)l * t->delay_send) / 127;
        send_r += ((int64_t)r * t->delay_send) / 127;
    }

    int16_t dl = m->delay_l[m->delay_pos];
    int16_t dr = m->delay_r[m->delay_pos];
    int32_t write_l = (int32_t)send_l + (int32_t)(((int64_t)dl * m->delay_feedback) >> 15);
    int32_t write_r = (int32_t)send_r + (int32_t)(((int64_t)dr * m->delay_feedback) >> 15);
    m->delay_l[m->delay_pos] = clip16(write_l);
    m->delay_r[m->delay_pos] = clip16(write_r);
    m->delay_pos++;
    if (m->delay_pos >= m->delay_len) m->delay_pos = 0u;

    left += ((int64_t)dl * m->delay_mix) >> 15;
    right += ((int64_t)dr * m->delay_mix) >> 15;

    int32_t l = saturate(clamp32(left, -65536, 65535), m->drive);
    int32_t r = saturate(clamp32(right, -65536, 65535), m->drive);
    l = compress(l, m->compressor_threshold);
    r = compress(r, m->compressor_threshold);

    int32_t fc = clamp32(m->filter_cutoff, 256, 32767);
    m->filter_l += (int32_t)(((int64_t)(l - m->filter_l) * fc) >> 15);
    m->filter_r += (int32_t)(((int64_t)(r - m->filter_r) * fc) >> 15);
    *out_l = clip16(m->filter_l);
    *out_r = clip16(m->filter_r);
}
