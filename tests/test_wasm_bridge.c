#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/proto/drum_machine.h"

void refm_wasm_init(uint16_t bpm);
void refm_wasm_midi(uint8_t byte);
void refm_wasm_render(int32_t *interleaved_stereo, uint32_t frames);
void refm_wasm_set_acid_wave(uint8_t track, uint8_t square);
void refm_wasm_set_acid_accent(uint8_t track, uint8_t value);
void refm_wasm_set_acid_drive(uint8_t track, uint8_t value);
void refm_wasm_set_acid_mod(uint8_t track, uint8_t param, uint8_t value);
uint16_t refm_wasm_acid_cutoff_hz(uint8_t track);
void refm_wasm_set_mix_track(uint8_t track, uint8_t param, int16_t value);
int16_t refm_wasm_get_mix_track(uint8_t track, uint8_t param);
void refm_wasm_set_fx(uint8_t param, int32_t value);
int32_t refm_wasm_get_fx(uint8_t param);
void refm_wasm_set_bpm(uint16_t bpm);
uint16_t refm_wasm_bpm(void);
void refm_wasm_enable_samples(uint8_t track, uint8_t enabled);
uint16_t refm_wasm_sample_mask(uint8_t track);
uint16_t refm_wasm_sample_use_mask(uint8_t track);
uint16_t refm_wasm_sample_active_mask(uint8_t track);
void refm_wasm_set_sample_lane(uint8_t track, uint8_t lane, uint8_t enabled);
void refm_wasm_set_sample_use_mask(uint8_t track, uint16_t mask);

static void test_303_controls(void) {
    int32_t a[256], b[256];
    refm_wasm_init(128u); assert(refm_wasm_acid_cutoff_hz(0u) >= 250u);
    refm_wasm_set_acid_accent(0u, 110u); refm_wasm_set_acid_drive(0u, 20u); refm_wasm_set_acid_mod(0u, 5u, 0u); refm_wasm_set_acid_wave(0u, 0u);
    refm_wasm_midi(0x90u); refm_wasm_midi(48u); refm_wasm_midi(110u); refm_wasm_render(a, 128u);
    refm_wasm_init(128u); refm_wasm_set_acid_wave(0u, 1u); refm_wasm_midi(0x90u); refm_wasm_midi(48u); refm_wasm_midi(110u); refm_wasm_render(b, 128u);
    int different = 0; for (unsigned i = 0; i < 256u; ++i) if (a[i] != b[i]) { different = 1; break; } assert(different);
}

static void test_mixer_bridge(void) {
    refm_wasm_init(128u);
    refm_wasm_set_mix_track(0u,0u,12345); assert(refm_wasm_get_mix_track(0u,0u)==12345);
    refm_wasm_set_mix_track(1u,1u,-5000); assert(refm_wasm_get_mix_track(1u,1u)==-5000);
    refm_wasm_set_mix_track(2u,2u,1); assert(refm_wasm_get_mix_track(2u,2u)==1);
    refm_wasm_set_mix_track(3u,3u,1); assert(refm_wasm_get_mix_track(3u,3u)==1);
    refm_wasm_set_mix_track(0u,4u,99); assert(refm_wasm_get_mix_track(0u,4u)==99);
    refm_wasm_set_fx(0u,4000); assert(refm_wasm_get_fx(0u)==4000);
    refm_wasm_set_fx(1u,20000); assert(refm_wasm_get_fx(1u)==20000);
    refm_wasm_set_fx(2u,22000); assert(refm_wasm_get_fx(2u)==22000);
    refm_wasm_set_fx(3u,12000); assert(refm_wasm_get_fx(3u)==12000);
    refm_wasm_set_fx(4u,6000); assert(refm_wasm_get_fx(4u)==6000);
    /* TIME is milliseconds now. Keep this assertion within the compact
       non-WASM buffer so the bridge test is valid for both host/FM-1-sized and
       long browser builds. */
    refm_wasm_set_fx(5u,25); assert(refm_wasm_get_fx(5u)==25);
}

static void test_tempo_bridge(void) {
    refm_wasm_init(128u);
    refm_wasm_set_bpm(166u); assert(refm_wasm_bpm()==166u);
    refm_wasm_set_bpm(1u); assert(refm_wasm_bpm()==30u);
    refm_wasm_set_bpm(999u); assert(refm_wasm_bpm()==300u);
}

static void test_sample_bridge_no_assets(void) {
    refm_wasm_init(128u); refm_wasm_enable_samples(0u,1u); refm_wasm_set_sample_use_mask(0u,0x7ffu);
    assert(refm_wasm_sample_use_mask(0u)==0x7ffu);
    assert(refm_wasm_sample_active_mask(0u)==(refm_wasm_sample_mask(0u)&0x7ffu));
    refm_wasm_set_sample_lane(0u,DRUM_VOICES-1u,0u);
    assert(!(refm_wasm_sample_use_mask(0u)&(1u<<(DRUM_VOICES-1u))));
}

int main(void) {
    test_303_controls();
    test_mixer_bridge();
    test_tempo_bridge();
    test_sample_bridge_no_assets();
    puts("wasm bridge tests: ok");
    return 0;
}
