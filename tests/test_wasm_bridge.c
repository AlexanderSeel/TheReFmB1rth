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
uint16_t refm_wasm_bpm(void);

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
    puts("WASM bridge tests: ok");
    return 0;
}
