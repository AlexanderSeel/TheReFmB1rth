// SPDX-License-Identifier: GPL-3.0-only
#include <stddef.h>
#include <stdint.h>
#include "../../firmware/integration/refm_target.h"
#include "../../firmware/proto/groovebox_pattern.h"
#include "../../firmware/proto/ui_graph_model.h"
#ifdef REFM_BUNDLED_SAMPLES
#include "refm_sample_assets.h"
#endif

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
#ifdef REFM_BUNDLED_SAMPLES
    refm_attach_bundled_samples(&vm.groovebox.drum[0], DRUM_MODEL_808);
    refm_attach_bundled_samples(&vm.groovebox.drum[1], DRUM_MODEL_909);
#endif
    drum_machine_enable_samples(&vm.groovebox.drum[0], 1u);
    drum_machine_enable_samples(&vm.groovebox.drum[1], 1u);
    project_size = 0u;
}
REFM_EXPORT void refm_wasm_external_clock(uint8_t enabled) { refm_target_set_external_clock(&vm, enabled); }
REFM_EXPORT void refm_wasm_midi(uint8_t byte) { refm_target_midi_byte(&vm, byte); }
REFM_EXPORT void refm_wasm_panic(void) { refm_target_panic(&vm); }
REFM_EXPORT void refm_wasm_render(int32_t *interleaved_stereo, uint32_t frames) { refm_target_render_q15(&vm, interleaved_stereo, frames); }

REFM_EXPORT int refm_wasm_snapshot(void) { size_t n = 0u; int rc = refm_target_save(&vm, project_buffer, sizeof(project_buffer), &n); if (rc == PROJECT_OK) project_size = n; return rc; }
REFM_EXPORT const uint8_t *refm_wasm_snapshot_ptr(void) { return project_buffer; }
REFM_EXPORT uint32_t refm_wasm_snapshot_size(void) { return (uint32_t)project_size; }
REFM_EXPORT int refm_wasm_restore(const uint8_t *data, uint32_t len) { return refm_target_load(&vm, data, len); }
REFM_EXPORT uint8_t refm_wasm_step(void) { return vm.groovebox.step; }
REFM_EXPORT uint8_t refm_wasm_pattern(void) { return vm.groovebox.current_pattern; }
REFM_EXPORT uint16_t refm_wasm_bpm(void) { return vm.groovebox.transport.bpm; }
REFM_EXPORT void refm_wasm_set_bpm(uint16_t bpm) { midi_transport_set_bpm(&vm.groovebox.transport, bpm); }

REFM_EXPORT void refm_wasm_enable_samples(uint8_t track, uint8_t enabled) { if (track < GROOVEBOX_DRUM_TRACKS) drum_machine_enable_samples(&vm.groovebox.drum[track], enabled); }
REFM_EXPORT uint16_t refm_wasm_sample_mask(uint8_t track) { return track < GROOVEBOX_DRUM_TRACKS ? vm.groovebox.drum[track].sample_mask : 0u; }
REFM_EXPORT uint16_t refm_wasm_sample_use_mask(uint8_t track) { return track < GROOVEBOX_DRUM_TRACKS ? vm.groovebox.drum[track].sample_use_mask : 0u; }
REFM_EXPORT uint16_t refm_wasm_sample_active_mask(uint8_t track) { return track < GROOVEBOX_DRUM_TRACKS ? drum_machine_sample_active_mask(&vm.groovebox.drum[track]) : 0u; }
REFM_EXPORT void refm_wasm_set_sample_lane(uint8_t track, uint8_t lane, uint8_t enabled) { if (track < GROOVEBOX_DRUM_TRACKS && lane < DRUM_VOICES) drum_machine_set_sample_lane(&vm.groovebox.drum[track], (drum_voice_id_t)lane, enabled); }
REFM_EXPORT void refm_wasm_set_sample_use_mask(uint8_t track, uint16_t mask) { if (track < GROOVEBOX_DRUM_TRACKS) drum_machine_set_sample_use_mask(&vm.groovebox.drum[track], mask); }
REFM_EXPORT void refm_wasm_select_pattern(uint8_t pattern) { pattern &= 7u; groovebox_pattern_store(&vm.groovebox.patterns, &vm.groovebox, vm.groovebox.current_pattern); groovebox_pattern_load(&vm.groovebox.patterns, &vm.groovebox, pattern); }

REFM_EXPORT void refm_wasm_set_acid_step(uint8_t track, uint8_t step, uint8_t note, uint8_t flags, uint8_t probability) {
    seq16_step_t *s; if (track >= 2u || step >= SEQ16_STEPS) return; s = &vm.groovebox.acid_seq[track].step[step];
    s->note = note; s->flags = flags & (SEQ16_GATE | SEQ16_ACCENT | SEQ16_SLIDE | SEQ16_TIE); s->probability = probability > 100u ? 100u : probability;
}
REFM_EXPORT uint32_t refm_wasm_get_acid_step(uint8_t track, uint8_t step) { const seq16_step_t *s; if (track >= 2u || step >= SEQ16_STEPS) return 0u; s=&vm.groovebox.acid_seq[track].step[step]; return (uint32_t)s->note | ((uint32_t)s->flags << 8) | ((uint32_t)s->probability << 16) | ((uint32_t)(uint8_t)s->micro << 24); }
REFM_EXPORT void refm_wasm_set_drum_step(uint8_t track, uint8_t step, uint16_t hits, uint16_t accents) { groovebox_set_drum_step(&vm.groovebox, track, step, hits, accents); }
REFM_EXPORT uint32_t refm_wasm_get_drum_step(uint8_t track, uint8_t step) { if (track >= 2u || step >= GROOVEBOX_STEPS) return 0u; return (uint32_t)vm.groovebox.drum_hits[track][step] | ((uint32_t)vm.groovebox.drum_accents[track][step] << 16); }

REFM_EXPORT void refm_wasm_set_acid_wave(uint8_t track, uint8_t square) { if (track < 2u) vm.groovebox.acid[track].square = square ? 1u : 0u; }
REFM_EXPORT void refm_wasm_set_acid_tune(uint8_t track, int8_t value) { if (track < 2u) vm.groovebox.acid[track].tune = value; }
REFM_EXPORT void refm_wasm_set_acid_accent(uint8_t track, uint8_t value) { if (track < 2u) vm.groovebox.acid[track].accent = (uint16_t)(value > 127u ? 127u : value) * 258u; }
REFM_EXPORT void refm_wasm_set_acid_drive(uint8_t track, uint8_t value) { if (track < 2u) vm.groovebox.acid[track].drive = (uint16_t)(value > 127u ? 127u : value) * 128u; }
REFM_EXPORT uint16_t refm_wasm_acid_cutoff_hz(uint8_t track) { return track < 2u ? acid303_cutoff_hz(&vm.groovebox.acid[track]) : 0u; }

REFM_EXPORT void refm_wasm_set_acid_mod(uint8_t track, uint8_t param, uint8_t value) {
    acid303_t *s; if (track >= 2u) return; s = &vm.groovebox.acid[track];
    if (param == 0u) s->amp_attack = value; else if (param == 1u) s->amp_decay = value; else if (param == 2u) s->amp_sustain = value; else if (param == 3u) s->amp_release = value;
    else if (param == 4u) s->lfo_rate = value; else if (param == 5u) s->lfo_amount = value; else if (param == 6u) s->lfo_shape = (uint8_t)(value & 3u);
}

REFM_EXPORT void refm_wasm_set_mix_track(uint8_t track, uint8_t param, int16_t value) {
    mixer_track_t *t; if (track >= MIX_TRACKS) return; t=&vm.groovebox.mixer.track[track];
    if (param == 0u) t->level = value < 0 ? 0 : value;
    else if (param == 1u) t->pan = value;
    else if (param == 2u) t->mute = value ? 1u : 0u;
    else if (param == 3u) t->solo = value ? 1u : 0u;
    else if (param == 4u) t->delay_send = (uint8_t)(value < 0 ? 0 : value > 127 ? 127 : value);
}
REFM_EXPORT void refm_wasm_set_fx(uint8_t param, int16_t value) {
    mixer_fx_t *m=&vm.groovebox.mixer;
    if (param == 0u) m->drive = value < 0 ? 0 : value;
    else if (param == 1u) m->compressor_threshold = value < 512 ? 512 : value;
    else if (param == 2u) m->filter_cutoff = value < 256 ? 256 : value;
    else if (param == 3u) mixer_fx_set_delay(m, m->delay_len, value, m->delay_mix);
    else if (param == 4u) mixer_fx_set_delay(m, m->delay_len, m->delay_feedback, value);
    else if (param == 5u) mixer_fx_set_delay(m, (uint16_t)(value < 1 ? 1 : value), m->delay_feedback, m->delay_mix);
}
REFM_EXPORT int16_t refm_wasm_get_mix_track(uint8_t track, uint8_t param) {
    const mixer_track_t *t;
    if (track >= MIX_TRACKS) return 0;
    t=&vm.groovebox.mixer.track[track];
    if (param == 0u) return t->level;
    if (param == 1u) return t->pan;
    if (param == 2u) return t->mute;
    if (param == 3u) return t->solo;
    if (param == 4u) return t->delay_send;
    return 0;
}
REFM_EXPORT int16_t refm_wasm_get_fx(uint8_t param) {
    const mixer_fx_t *m=&vm.groovebox.mixer;
    if (param == 0u) return m->drive;
    if (param == 1u) return m->compressor_threshold;
    if (param == 2u) return m->filter_cutoff;
    if (param == 3u) return m->delay_feedback;
    if (param == 4u) return m->delay_mix;
    if (param == 5u) return (int16_t)m->delay_len;
    return 0;
}

REFM_EXPORT const int16_t *refm_wasm_graph(uint8_t track, uint8_t kind) {
    acid303_t *s; uint8_t c, r, e; if (track >= 2u) track = 0u; s = &vm.groovebox.acid[track];
    if (kind == 1u) ui_graph_adsr(&graph_curve, 0u, (uint8_t)(s->decay >> 8), 0u, 0u);
    else if (kind == 2u) ui_graph_lfo(&graph_curve, (ui_lfo_shape_t)(s->lfo_shape & 3u), (uint8_t)(s->lfo_phase >> 24), s->lfo_amount);
    else { c = (uint8_t)(s->cutoff >> 8); r = (uint8_t)(s->resonance >> 8); e = (uint8_t)(s->env_mod >> 8); ui_graph_filter(&graph_curve, c, r, e); }
    return graph_curve.y;
}