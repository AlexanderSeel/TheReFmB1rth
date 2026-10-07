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

static void test_cutoff_mapping(void) {
    acid303_t s; acid303_init(&s);
    s.cutoff = 0u;
    assert(acid303_cutoff_hz(&s) >= 240u && acid303_cutoff_hz(&s) <= 300u);
    s.cutoff = 32767u;
    assert(acid303_cutoff_hz(&s) >= 2300u && acid303_cutoff_hz(&s) <= 2500u);
}

static void test_slide_is_legato_and_converges(void) {
    acid303_t s; acid303_init(&s); acid303_set_note(&s, 48, 0, 0);
    for (int i = 0; i < 100; ++i) acid303_process(&s);
    int32_t env_before = s.env;
    uint32_t start = s.phase_inc;
    acid303_set_note(&s, 60, 0, 1);
    uint32_t target = s.slide_target_inc;
    assert(target > start);
    assert(s.env == env_before);
    for (int i = 0; i < 4000; ++i) acid303_process(&s);
    assert(s.phase_inc > start);
    assert(s.phase_inc <= target);
}

static void test_note_off_closes_vca(void) {
    acid303_t s; acid303_init(&s); acid303_set_note(&s, 52, 0, 0);
    for (int i = 0; i < 100; ++i) acid303_process(&s);
    acid303_note_off(&s);
    for (int i = 0; i < 20000; ++i) acid303_process(&s);
    assert(s.amp == 0);
    assert(s.amp_stage == ACID_ENV_OFF);
}

static void test_accent_sweep_accumulates(void) {
    acid303_t s; acid303_init(&s);
    acid303_set_note(&s, 48, 1, 0);
    int32_t first = s.accent_sweep;
    for (int i = 0; i < 1000; ++i) acid303_process(&s);
    acid303_note_off(&s);
    acid303_set_note(&s, 48, 1, 0);
    assert(s.accent_sweep > first);
}

static void test_saw_and_303_square_are_different(void) {
    acid303_t a, b; acid303_init(&a); acid303_init(&b);
    a.square = 0u; b.square = 1u;
    acid303_set_note(&a, 48, 0, 0); acid303_set_note(&b, 48, 0, 0);
    int different = 0;
    for (int i = 0; i < 512; ++i) {
        if (acid303_process(&a) != acid303_process(&b)) { different = 1; break; }
    }
    assert(different);
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
    }
    assert(changed == 4);
}

static uint64_t render_shape_hash(uint8_t shape) {
    acid303_t s; acid303_init(&s);
    s.lfo_shape = shape; s.lfo_amount = 110u; s.lfo_rate = 90u;
    acid303_set_note(&s, 48, 0, 0);
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < 12000; ++i) {
        uint16_t y = (uint16_t)acid303_process(&s);
        h ^= (uint8_t)y; h *= 1099511628211ull;
        h ^= (uint8_t)(y >> 8); h *= 1099511628211ull;
    }
    return h;
}

static void test_lfo_shapes_change_audio(void) {
    uint64_t h[4];
    for (uint8_t i = 0; i < 4u; ++i) h[i] = render_shape_hash(i);
    for (unsigned i = 0; i < 4u; ++i)
        for (unsigned j = i + 1u; j < 4u; ++j)
            assert(h[i] != h[j]);
}

int main(void) {
    test_note_order();
    test_output_is_bounded();
    test_cutoff_mapping();
    test_slide_is_legato_and_converges();
    test_note_off_closes_vca();
    test_accent_sweep_accumulates();
    test_saw_and_303_square_are_different();
    test_lfo_runs_all_shapes();
    test_lfo_shapes_change_audio();
    puts("acid303 circuit-model tests: ok");
    return 0;
}
