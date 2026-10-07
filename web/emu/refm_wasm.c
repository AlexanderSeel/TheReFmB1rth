// SPDX-License-Identifier: GPL-3.0-only
#include <stddef.h>
#include <stdint.h>
#include "../../firmware/integration/refm_target.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define REFM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define REFM_EXPORT
#endif

static refm_target_t vm;
static uint8_t project_buffer[PROJECT_STORE_MAX_PAYLOAD];
static size_t project_size;

REFM_EXPORT void refm_wasm_init(uint16_t bpm) {
    refm_target_init(&vm, bpm);
    project_size = 0u;
}

REFM_EXPORT void refm_wasm_external_clock(uint8_t enabled) {
    refm_target_set_external_clock(&vm, enabled);
}

REFM_EXPORT void refm_wasm_midi(uint8_t byte) {
    refm_target_midi_byte(&vm, byte);
}

REFM_EXPORT void refm_wasm_panic(void) {
    refm_target_panic(&vm);
}

REFM_EXPORT void refm_wasm_render(int32_t *interleaved_stereo, uint32_t frames) {
    refm_target_render_q15(&vm, interleaved_stereo, frames);
}

REFM_EXPORT int refm_wasm_snapshot(void) {
    size_t n = 0u;
    int rc = refm_target_save(&vm, project_buffer, sizeof(project_buffer), &n);
    if (rc == PROJECT_OK) project_size = n;
    return rc;
}

REFM_EXPORT const uint8_t *refm_wasm_snapshot_ptr(void) {
    return project_buffer;
}

REFM_EXPORT uint32_t refm_wasm_snapshot_size(void) {
    return (uint32_t)project_size;
}

REFM_EXPORT int refm_wasm_restore(const uint8_t *data, uint32_t len) {
    return refm_target_load(&vm, data, len);
}

REFM_EXPORT uint8_t refm_wasm_step(void) {
    return vm.groovebox.step;
}

REFM_EXPORT uint8_t refm_wasm_pattern(void) {
    return vm.groovebox.current_pattern;
}

REFM_EXPORT uint16_t refm_wasm_bpm(void) {
    return vm.groovebox.transport.bpm;
}
