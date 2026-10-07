// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/proto/acid303.h"

static void test_note_order(void) {
    assert(acid303_note_to_phase_inc(60) < acid303_note_to_phase_inc(61));
    assert(acid303_note_to_phase_inc(48) < acid303_note_to_phase_inc(60));
}

static void test_output_is_bounded(void) {
    acid303_t s; acid303_init(&s); acid303_set_note(&s, 48, 1, 0);
    s.lfo_amount = 127u;
    for (int i = 0; i < 44100 * 2; ++i) {
        int32_t y = acid303_process(&s);
        assert(y >= -32768 && y <= 32767);
    }
}

static void test_slide_converges(void) {
    acid303_t s; acid303_init(&s); acid303_set_note(&s, 48, 0, 0);
    uint32_t start = s.phase_inc;
    acid303_set_note(&s, 60, 0, 1);
    uint32_t target = s.slide_target_inc;
    assert(target > start);
    for (int i = 0; i < 4000; ++i) acid303_process(&s);
    assert(s.phase_inc > start);
    assert(s.phase_inc <= target);
}

static void test_release_decays(void) {
    acid303_t s; acid303_init(&s); acid303_set_note(&s, 52, 0, 0);
    for (int i = 0; i < 100; ++i) acid303_process(&s);
    acid303_note_off(&s);
    for (int i = 0; i < 1000; ++i) acid303_process(&s);
    assert(s.amp == 0);
    assert(s.amp_stage == ACID_ENV_OFF);
}

static void test_adsr_sustain(void) {
    acid303_t s; acid303_init(&s);
    s.amp_attack = 30u;
    s.amp_decay = 20u;
    s.amp_sustain = 64u;
    acid303_set_note(&s, 52, 0, 0);
    assert(s.amp_stage == ACID_ENV_ATTACK);
    for (int i = 0; i < 12000; ++i) acid303_process(&s);
    assert(s.amp_stage == ACID_ENV_SUSTAIN || s.amp_stage == ACID_ENV_DECAY);
    assert(s.amp > 15000 && s.amp < 20000);
}

static void test_lfo_runs_all_shapes(void) {
    acid303_t s; acid303_init(&s);
    int changed = 0;
    for (uint8_t shape = 0; shape < 4u; ++shape) {
        s.lfo_shape = shape;
        s.lfo_phase = 0u;
        int16_t first = acid303_lfo_value(&s);
        for (int i = 0; i < 4096; ++i) acid303_process(&s);
        int16_t later = acid303_lfo_value(&s);
        if (later != first) changed++;
        assert(first >= -32768 && first <= 32767);
        assert(later >= -32768 && later <= 32767);
    }
    assert(changed == 4);
}

int main(void) {
    test_note_order();
    test_output_is_bounded();
    test_slide_converges();
    test_release_decays();
    test_adsr_sustain();
    test_lfo_runs_all_shapes();
    puts("acid303 prototype tests: ok");
    return 0;
}
