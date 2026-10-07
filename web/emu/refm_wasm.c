// SPDX-License-Identifier: GPL-3.0-only
#include <stddef.h>
#include <stdint.h>
#include "../../firmware/integration/refm_target.h"
#include "../../firmware/proto/ui_graph_model.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define REFM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define REFM_EXPORT
#endif

static refm_target_t vm;
static uint8_t project_buffer[PROJECT_STORE_MAX_PAYLOAD];
static size_t project_size;
static ui_graph_curve_t graph_curve;

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

/* Browser/developer controls that map directly to real ACID modulation state.
 * param: 0 attack, 1 decay, 2 sustain, 3 release, 4 lfo rate,
 *        5 lfo amount, 6 lfo shape. Values are 0..127 except shape 0..3. */
REFM_EXPORT void refm_wasm_set_acid_mod(uint8_t track, uint8_t param, uint8_t value) {
    acid303_t *s;
    if (track >= 2u) return;
    s = &vm.groovebox.acid[track];
    if (param == 0u) s->amp_attack = value;
    else if (param == 1u) s->amp_decay = value;
    else if (param == 2u) s->amp_sustain = value;
    else if (param == 3u) s->amp_release = value;
    else if (param == 4u) s->lfo_rate = value;
    else if (param == 5u) s->lfo_amount = value;
    else if (param == 6u) s->lfo_shape = (uint8_t)(value & 3u);
}

/* kind: 0 filter response, 1 ADSR, 2 LFO. Returns 64 signed Q15-ish points. */
REFM_EXPORT const int16_t *refm_wasm_graph(uint8_t track, uint8_t kind) {
    acid303_t *s;
    uint8_t c, r, e;
    if (track >= 2u) track = 0u;
    s = &vm.groovebox.acid[track];
    if (kind == 1u) {
        ui_graph_adsr(&graph_curve, s->amp_attack, s->amp_decay, s->amp_sustain, s->amp_release);
    } else if (kind == 2u) {
        ui_graph_lfo(&graph_curve, (ui_lfo_shape_t)(s->lfo_shape & 3u), (uint8_t)(s->lfo_phase >> 24), s->lfo_amount);
    } else {
        c = (uint8_t)(s->cutoff >> 9);
        r = (uint8_t)(s->resonance >> 9);
        e = (uint8_t)(s->env_mod >> 9);
        ui_graph_filter(&graph_curve, c, r, e);
    }
    return graph_curve.y;
}
