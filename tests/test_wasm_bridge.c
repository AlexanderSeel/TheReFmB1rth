// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

void refm_wasm_init(uint16_t bpm);
void refm_wasm_external_clock(uint8_t enabled);
void refm_wasm_midi(uint8_t byte);
void refm_wasm_render(int32_t *interleaved_stereo, uint32_t frames);
int refm_wasm_snapshot(void);
const uint8_t *refm_wasm_snapshot_ptr(void);
uint32_t refm_wasm_snapshot_size(void);
int refm_wasm_restore(const uint8_t *data, uint32_t len);
uint8_t refm_wasm_step(void);
uint8_t refm_wasm_pattern(void);
uint16_t refm_wasm_bpm(void);
void refm_wasm_select_pattern(uint8_t pattern);
void refm_wasm_set_acid_step(uint8_t track, uint8_t step, uint8_t note, uint8_t flags, uint8_t probability);
uint32_t refm_wasm_get_acid_step(uint8_t track, uint8_t step);
void refm_wasm_set_drum_step(uint8_t track, uint8_t step, uint16_t hits, uint16_t accents);
uint32_t refm_wasm_get_drum_step(uint8_t track, uint8_t step);
void refm_wasm_set_acid_wave(uint8_t track, uint8_t square);
void refm_wasm_set_acid_accent(uint8_t track, uint8_t value);
void refm_wasm_set_acid_drive(uint8_t track, uint8_t value);
uint16_t refm_wasm_acid_cutoff_hz(uint8_t track);
void refm_wasm_set_acid_mod(uint8_t track, uint8_t param, uint8_t value);

static void test_pattern_roundtrip(void) {
    uint32_t packed;
    refm_wasm_init(128u);
    refm_wasm_set_acid_step(0u, 3u, 48u, 3u, 87u);
    packed = refm_wasm_get_acid_step(0u, 3u);
    assert((packed & 0xffu) == 48u);
    assert(((packed >> 8) & 0xffu) == 3u);
    assert(((packed >> 16) & 0xffu) == 87u);

    refm_wasm_set_drum_step(0u, 4u, (uint16_t)((1u << 0) | (1u << 4)), (uint16_t)(1u << 4));
    packed = refm_wasm_get_drum_step(0u, 4u);
    assert((packed & 0xffffu) == ((1u << 0) | (1u << 4)));
    assert(((packed >> 16) & 0xffffu) == (1u << 4));

    refm_wasm_select_pattern(1u);
    assert(refm_wasm_pattern() == 1u);
    refm_wasm_set_acid_step(0u, 3u, 55u, 1u, 100u);
    refm_wasm_set_drum_step(0u, 4u, (uint16_t)(1u << 1), 0u);
    refm_wasm_select_pattern(0u);
    assert(refm_wasm_pattern() == 0u);
    packed = refm_wasm_get_acid_step(0u, 3u);
    assert((packed & 0xffu) == 48u);
    assert(((packed >> 8) & 0xffu) == 3u);
    packed = refm_wasm_get_drum_step(0u, 4u);
    assert((packed & 0xffffu) == ((1u << 0) | (1u << 4)));

    refm_wasm_select_pattern(1u);
    packed = refm_wasm_get_acid_step(0u, 3u);
    assert((packed & 0xffu) == 55u);
    packed = refm_wasm_get_drum_step(0u, 4u);
    assert((packed & 0xffffu) == (1u << 1));
}

static void test_303_controls(void) {
    int32_t a[256], b[256];
    refm_wasm_init(128u);
    assert(refm_wasm_acid_cutoff_hz(0u) >= 250u);
    refm_wasm_set_acid_accent(0u, 110u);
    refm_wasm_set_acid_drive(0u, 20u);
    refm_wasm_set_acid_mod(0u, 5u, 0u);
    refm_wasm_set_acid_wave(0u, 0u);
    refm_wasm_midi(0x90u); refm_wasm_midi(48u); refm_wasm_midi(110u);
    refm_wasm_render(a, 128u);
    refm_wasm_init(128u);
    refm_wasm_set_acid_wave(0u, 1u);
    refm_wasm_midi(0x90u); refm_wasm_midi(48u); refm_wasm_midi(110u);
    refm_wasm_render(b, 128u);
    int different = 0;
    for (unsigned i = 0; i < 256u; ++i) if (a[i] != b[i]) { different = 1; break; }
    assert(different);
}

int main(void) {
    int32_t audio[64 * 2];
    refm_wasm_init(142u);
    assert(refm_wasm_bpm() == 142u);
    refm_wasm_external_clock(1u);
    refm_wasm_midi(0xFAu);
    for (unsigned i = 0; i < 6u; ++i) refm_wasm_midi(0xF8u);
    assert(refm_wasm_step() != 0u);
    refm_wasm_midi(0x90u);
    refm_wasm_midi(48u);
    refm_wasm_midi(110u);
    refm_wasm_render(audio, 64u);
    for (unsigned i = 0; i < 128u; ++i) assert(audio[i] >= -32768 && audio[i] <= 32767);
    assert(refm_wasm_snapshot() == 0);
    assert(refm_wasm_snapshot_size() > 0u);
    assert(refm_wasm_restore(refm_wasm_snapshot_ptr(), refm_wasm_snapshot_size()) == 0);
    test_pattern_roundtrip();
    test_303_controls();
    puts("WASM bridge tests: ok");
    return 0;
}
