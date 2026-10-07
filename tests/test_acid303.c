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
}

int main(void) {
    test_note_order();
    test_output_is_bounded();
    test_slide_converges();
    test_release_decays();
    puts("acid303 prototype tests: ok");
    return 0;
}
