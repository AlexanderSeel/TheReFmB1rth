// SPDX-License-Identifier: GPL-3.0-only
#include <stddef.h>
#include <stdint.h>
#include "../../firmware/integration/refm_target.h"
#include "../../firmware/proto/groovebox_pattern.h"
#include "../../firmware/proto/ui_graph_model.h"
#include "../../firmware/proto/ui240.h"
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
static ui240_state_t panel_ui;
static ui240_view_t panel_view;
static uint8_t project_buffer[PROJECT_STORE_MAX_PAYLOAD];
static size_t project_size;
static ui_graph_curve_t graph_curve;

static void seed_beta_pattern(void) {
    groovebox_t *g = &vm.groovebox;
    static const uint8_t acid_a[16] = {36,36,43,36,39,36,46,43,36,48,43,39,36,43,46,34};
    static const uint8_t acid_b[16] = {48,0,0,0,0,0,43,0,46,0,0,0,0,0,39,0};
    uint8_t i;
    for (i = 0u; i < 16u; ++i) {
        seq16_step_t *a = &g->acid_seq[0].step[i];
        seq16_step_t *b = &g->acid_seq[1].step[i];
        a->note = acid_a[i]; a->flags = SEQ16_GATE; a->probability = 100u;
        if ((i & 3u) == 0u) a->flags |= SEQ16_ACCENT;
        if (i == 2u || i == 6u || i == 13u) a->flags |= SEQ16_SLIDE;
        b->note = acid_b[i] ? acid_b[i] : 48u;
        b->flags = acid_b[i] ? (uint8_t)(SEQ16_GATE | ((i == 0u || i == 8u) ? SEQ16_ACCENT : 0u)) : 0u;
        b->probability = 100u;
        if ((i & 3u) == 0u) g->drum_hits[0][i] |= (uint16_t)(1u << DRUM_BD);
        if (i == 4u || i == 12u) g->drum_hits[0][i] |= (uint16_t)(1u << DRUM_SD);
        if ((i & 1u) == 0u) g->drum_hits[0][i] |= (uint16_t)(1u << DRUM_CH);
        if (i == 0u || i == 8u) g->drum_accents[0][i] |= (uint16_t)(1u << DRUM_BD);
    }
    groovebox_pattern_store(&g->patterns, g, 0u);
}

REFM_EXPORT void refm_wasm_init(uint16_t bpm) {
    refm_target_init(&vm, bpm);
    ui240_init(&panel_ui);
    seed_beta_pattern();
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
    else if (param == 5u) t->reverb_send = (uint8_t)(value < 0 ? 0 : value > 127 ? 127 : value);
}

REFM_EXPORT void refm_wasm_set_fx(uint8_t param, int32_t value) {
    mixer_fx_t *m=&vm.groovebox.mixer;
    if (param == 0u) m->drive = (int16_t)(value < 0 ? 0 : value > 20000 ? 20000 : value);
    else if (param == 1u) m->compressor_threshold = (int16_t)(value < 512 ? 512 : value > 32767 ? 32767 : value);
    else if (param == 2u) m->filter_cutoff = (int16_t)(value < 256 ? 256 : value > 32767 ? 32767 : value);
    else if (param == 3u) mixer_fx_set_delay(m, m->delay_len, (int16_t)value, m->delay_mix);
    else if (param == 4u) mixer_fx_set_delay(m, m->delay_len, m->delay_feedback, (int16_t)value);
    else if (param == 5u) {
        uint32_t ms=(uint32_t)(value < 1 ? 1 : value > 1486 ? 1486 : value);
        uint32_t samples=(ms * 441u + 5u) / 10u;
        if (samples > MIX_DELAY_MAX) samples = MIX_DELAY_MAX;
        mixer_fx_set_delay(m, (uint16_t)samples, m->delay_feedback, m->delay_mix);
    }
    else if (param == 6u) mixer_fx_set_reverb(m, (int16_t)value, m->reverb_mix, m->reverb_damp);
    else if (param == 7u) mixer_fx_set_reverb(m, m->reverb_feedback, (int16_t)value, m->reverb_damp);
    else if (param == 8u) mixer_fx_set_reverb(m, m->reverb_feedback, m->reverb_mix, (int16_t)value);
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
    if (param == 5u) return t->reverb_send;
    return 0;
}
REFM_EXPORT int32_t refm_wasm_get_fx(uint8_t param) {
    const mixer_fx_t *m=&vm.groovebox.mixer;
    if (param == 0u) return m->drive;
    if (param == 1u) return m->compressor_threshold;
    if (param == 2u) return m->filter_cutoff;
    if (param == 3u) return m->delay_feedback;
    if (param == 4u) return m->delay_mix;
    if (param == 5u) return ((int32_t)m->delay_len * 1000 + 22050) / 44100;
    if (param == 6u) return m->reverb_feedback;
    if (param == 7u) return m->reverb_mix;
    if (param == 8u) return m->reverb_damp;
    return 0;
}

REFM_EXPORT const int16_t *refm_wasm_graph(uint8_t track, uint8_t kind) {
    acid303_t *s; uint8_t c, r, e; if (track >= 2u) track = 0u; s = &vm.groovebox.acid[track];
    if (kind == 1u) ui_graph_adsr(&graph_curve, 0u, (uint8_t)(s->decay >> 8), 0u, 0u);
    else if (kind == 2u) ui_graph_lfo(&graph_curve, (ui_lfo_shape_t)(s->lfo_shape & 3u), (uint8_t)(s->lfo_phase >> 24), s->lfo_amount);
    else { c = (uint8_t)(s->cutoff >> 8); r = (uint8_t)(s->resonance >> 8); e = (uint8_t)(s->env_mod >> 8); ui_graph_filter(&graph_curve, c, r, e); }
    return graph_curve.y;
}

/* Virtual FM-1 front-panel bridge. The action/encoder ids intentionally mirror
   refm_felucca_bridge.c so browser panel tests exercise the physical beta UX. */
static int8_t panel_sign(int16_t delta) { return delta < 0 ? -1 : delta > 0 ? 1 : 0; }
static void panel_pattern_delta(int8_t d) {
    uint8_t next;
    if (!d) return;
    groovebox_pattern_store(&vm.groovebox.patterns, &vm.groovebox, vm.groovebox.current_pattern);
    next=(uint8_t)((vm.groovebox.current_pattern + (d > 0 ? 1u : 7u)) & 7u);
    groovebox_pattern_load(&vm.groovebox.patterns, &vm.groovebox, next);
}
REFM_EXPORT void refm_wasm_panel_encoder(uint8_t encoder, int16_t delta) {
    int8_t d=panel_sign(delta); unsigned n;
    if (!d) return; n=(unsigned)(delta < 0 ? -delta : delta); if (n > 8u) n=8u;
    while (n--) {
        if (encoder == 4u) { if (panel_ui.page==UI240_ACID_STEP || panel_ui.page==UI240_DRUM) ui240_select_step(&panel_ui,d); else if(panel_ui.page==UI240_PROJECT) ui240_select_project(&panel_ui,d); else panel_pattern_delta(d); continue; }
        if (encoder == 5u) { ui240_cycle_machine(&panel_ui,d); continue; }
        if (encoder == 6u) { int32_t bpm=(int32_t)vm.groovebox.transport.bpm+d; if(bpm<30)bpm=30; if(bpm>300)bpm=300; midi_transport_set_bpm(&vm.groovebox.transport,(uint16_t)bpm); continue; }
        if (encoder > 3u) continue;
        if (panel_ui.page==UI240_ACID_MAIN) ui240_edit_acid_main(&panel_ui,&vm.groovebox,encoder,d);
        else if (panel_ui.page==UI240_ACID_STEP) ui240_edit_acid_step(&panel_ui,&vm.groovebox,encoder,d);
        else if (panel_ui.page==UI240_DRUM) ui240_edit_drum(&panel_ui,&vm.groovebox,encoder,d);
        else if (panel_ui.page==UI240_MIXER) ui240_edit_mixer(&panel_ui,&vm.groovebox,encoder,d);
        else if (panel_ui.page==UI240_FX) ui240_edit_fx(&panel_ui,&vm.groovebox,encoder,d);
        else if (panel_ui.page==UI240_PROJECT && encoder==0u) ui240_select_project(&panel_ui,d);
    }
}
REFM_EXPORT void refm_wasm_panel_action(uint8_t action) {
    groovebox_t *g=&vm.groovebox;
    switch(action) {
    case 0u: ui240_set_page(&panel_ui,UI240_HOME); break;
    case 1u: ui240_set_machine(&panel_ui,UI240_MACHINE_ACID1); ui240_set_page(&panel_ui,UI240_ACID_MAIN); break;
    case 2u: ui240_set_machine(&panel_ui,UI240_MACHINE_ACID2); ui240_set_page(&panel_ui,UI240_ACID_MAIN); break;
    case 3u: ui240_set_page(&panel_ui,panel_ui.machine>=UI240_MACHINE_808?UI240_DRUM:UI240_ACID_STEP); break;
    case 4u: if(panel_ui.machine<UI240_MACHINE_808)ui240_set_machine(&panel_ui,UI240_MACHINE_808);else ui240_cycle_machine(&panel_ui,1);ui240_set_page(&panel_ui,UI240_DRUM); break;
    case 5u: ui240_set_page(&panel_ui,UI240_MIXER); break;
    case 6u: ui240_set_page(&panel_ui,UI240_FX); break;
    case 7u: ui240_set_page(&panel_ui,UI240_SONG); break;
    case 8u: ui240_set_page(&panel_ui,UI240_PROJECT); break;
    case 9u: refm_target_set_external_clock(&vm,0u); refm_target_midi_byte(&vm,g->transport.playing?0xFCu:0xFAu); break;
    case 10u:
        if(panel_ui.page==UI240_ACID_MAIN && panel_ui.machine<UI240_MACHINE_808) g->acid[panel_ui.machine].square^=1u;
        else if(panel_ui.page==UI240_ACID_STEP && panel_ui.machine<UI240_MACHINE_808) ui240_toggle_acid_flag(&panel_ui,g,SEQ16_ACCENT);
        else if(panel_ui.page==UI240_DRUM) ui240_edit_drum(&panel_ui,g,1u,1);
        else if(panel_ui.page==UI240_MIXER) g->mixer.track[panel_ui.machine&3u].mute^=1u;
        else if(panel_ui.page==UI240_SONG) g->song.loop^=1u;
        break;
    case 11u:
        if(panel_ui.page==UI240_ACID_STEP && panel_ui.machine<UI240_MACHINE_808) ui240_toggle_acid_flag(&panel_ui,g,SEQ16_SLIDE);
        else if(panel_ui.page==UI240_MIXER) g->mixer.track[panel_ui.machine&3u].solo^=1u;
        break;
    case 14u: refm_target_panic(&vm); break;
    default: break;
    }
}
REFM_EXPORT void refm_wasm_panel_key(uint8_t note, uint8_t on) {
    uint8_t status;
    if(panel_ui.machine>=UI240_MACHINE_808 || panel_ui.page==UI240_ACID_STEP || panel_ui.page==UI240_DRUM || panel_ui.page==UI240_SONG || panel_ui.page==UI240_PROJECT || panel_ui.page==UI240_EXT) return;
    status=(uint8_t)((on?0x90u:0x80u)|panel_ui.machine); refm_target_midi_byte(&vm,status); refm_target_midi_byte(&vm,note); refm_target_midi_byte(&vm,on?108u:0u);
}
REFM_EXPORT uint32_t refm_wasm_panel_value(uint8_t field) {
    ui240_snapshot(&panel_ui,&vm.groovebox,&panel_view);
    switch(field) {
    case 0u:return panel_view.page; case 1u:return panel_view.machine; case 2u:return panel_view.pattern; case 3u:return panel_view.playing;
    case 4u:return panel_view.playing_step; case 5u:return panel_view.selected_step; case 6u:return panel_view.selected_lane; case 7u:return panel_view.project_slot;
    case 8u:return panel_view.bpm; case 9u:return panel_view.p0; case 10u:return panel_view.p1; case 11u:return panel_view.p2; case 12u:return panel_view.p3;
    case 13u:return panel_view.wave_square; case 14u:return panel_view.step_type; case 15u:return panel_view.accent; case 16u:return panel_view.slide;
    case 17u:return panel_view.step_probability; case 18u:return panel_view.drum_state; case 19u:return panel_view.mute; case 20u:return panel_view.solo;
    default:return 0u;
    }
}
