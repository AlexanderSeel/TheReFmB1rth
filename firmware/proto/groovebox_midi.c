// SPDX-License-Identifier: GPL-3.0-only
#include "groovebox_midi.h"

static uint16_t cc16(uint8_t v) { return (uint16_t)v * 258u; }
static int16_t pan16(uint8_t v) { return (int16_t)(((int32_t)v - 64) * 512); }

void groovebox_midi_init(midi_router_t *r) { midi_router_init(r); }

/* Live-editor CC contract. Keep the classic/common controls where practical and
   reserve a compact group for TheReFmB1rth-specific parameters:
   1 env mod, 7 level, 10 pan, 71 resonance, 73 decay, 74 cutoff,
   75 accent, 76 acid distortion, 77 wave (0..63 saw / 64..127 square),
   91 reverb send, 94 delay send, 102 editor step, 103 note,
   104 acid step type / drum voice, 105 acid flags / drum state,
   106 acid probability, 119 solo, 120 mute.

   Acid step type: 0=REST, 1=NOTE, 2=TIE.
   Acid flags: bit0=accent, bit1=slide. Drum state: 0=off, 1=hit, 2=accent. */
static void track_mix_cc(groovebox_t *g, unsigned track, uint8_t cc, uint8_t v)
{
    mixer_track_t *m = &g->mixer.track[track];
    if (cc == 7u) m->level = (int16_t)cc16(v);
    else if (cc == 10u) m->pan = pan16(v);
    else if (cc == 91u) m->reverb_send = v;
    else if (cc == 94u) m->delay_send = v;
    else if (cc == 119u) m->solo = v >= 64u;
    else if (cc == 120u) m->mute = v >= 64u;
}

static void acid_step_cc(groovebox_t *g, midi_router_t *r, unsigned a, uint8_t cc, uint8_t v)
{
    seq16_step_t *st = &g->acid_seq[a].step[r->editor_step & 15u];
    if (cc == 102u) r->editor_step = v & 15u;
    else if (cc == 103u) st->note = v;
    else if (cc == 104u) {
        st->flags &= (uint8_t)~(SEQ16_GATE | SEQ16_TIE);
        if (v == 1u) st->flags |= SEQ16_GATE;
        else if (v >= 2u) st->flags |= SEQ16_TIE;
    } else if (cc == 105u) {
        st->flags &= (uint8_t)~(SEQ16_ACCENT | SEQ16_SLIDE);
        if (v & 1u) st->flags |= SEQ16_ACCENT;
        if (v & 2u) st->flags |= SEQ16_SLIDE;
    } else if (cc == 106u) st->probability = v > 100u ? 100u : v;
}

static void acid_cc(groovebox_t *g, midi_router_t *r, unsigned a, uint8_t cc, uint8_t v)
{
    acid303_t *s = &g->acid[a];
    if (cc >= 102u && cc <= 106u) acid_step_cc(g, r, a, cc, v);
    else if (cc == 74u) s->cutoff = cc16(v);
    else if (cc == 71u) s->resonance = cc16(v);
    else if (cc == 73u) s->decay = cc16(v);
    else if (cc == 1u) s->env_mod = cc16(v);
    else if (cc == 75u) s->accent = cc16(v);
    else if (cc == 76u) s->drive = cc16(v);
    else if (cc == 77u) s->square = v >= 64u;
    else track_mix_cc(g, a, cc, v);
}

static void drum_step_cc(groovebox_t *g, midi_router_t *r, unsigned d, uint8_t cc, uint8_t v)
{
    uint8_t step = r->editor_step & 15u;
    if (cc == 102u) r->editor_step = v & 15u;
    else if (cc == 104u) r->editor_voice = v > 10u ? 10u : v;
    else if (cc == 105u) {
        uint16_t bit = (uint16_t)(1u << r->editor_voice);
        if (v == 0u) {
            g->drum_hits[d][step] &= (uint16_t)~bit;
            g->drum_accents[d][step] &= (uint16_t)~bit;
        } else {
            g->drum_hits[d][step] |= bit;
            if (v >= 2u) g->drum_accents[d][step] |= bit;
            else g->drum_accents[d][step] &= (uint16_t)~bit;
        }
    }
}

uint8_t groovebox_handle_channel_midi(groovebox_t *g, midi_router_t *r, uint8_t st, uint8_t d1, uint8_t d2)
{
    midi_route_event_t e = midi_router_decode(r, st, d1, d2);
    if (e.type == MIDI_EVENT_NONE) return 0u;

    if (e.target == MIDI_TARGET_ACID1 || e.target == MIDI_TARGET_ACID2) {
        unsigned a = e.target == MIDI_TARGET_ACID1 ? 0u : 1u;
        if (e.type == MIDI_EVENT_NOTE_ON) acid303_set_note(&g->acid[a], e.data1, e.data2 > 100u, 0u);
        else if (e.type == MIDI_EVENT_NOTE_OFF) acid303_note_off(&g->acid[a]);
        else if (e.type == MIDI_EVENT_CC) acid_cc(g, r, a, e.data1, e.data2);
        else if (e.type == MIDI_EVENT_PROGRAM) g->current_pattern = (uint8_t)(e.data1 & 7u);
        return 1u;
    }

    if (e.target == MIDI_TARGET_DRUM808 || e.target == MIDI_TARGET_DRUM909) {
        unsigned d = e.target == MIDI_TARGET_DRUM808 ? 0u : 1u;
        unsigned track = d + 2u;
        if (e.type == MIDI_EVENT_NOTE_ON) {
            int v = midi_router_drum_voice(e.data1);
            if (v >= 0) drum_machine_trigger(&g->drum[d], (drum_voice_id_t)v, e.data2);
        } else if (e.type == MIDI_EVENT_CC) {
            if (e.data1 >= 102u && e.data1 <= 105u) drum_step_cc(g, r, d, e.data1, e.data2);
            else track_mix_cc(g, track, e.data1, e.data2);
            if (e.data1 == 20u && e.data2 >= 64u)
                r->channel10_target = e.target == MIDI_TARGET_DRUM808 ? MIDI_TARGET_DRUM909 : MIDI_TARGET_DRUM808;
        } else if (e.type == MIDI_EVENT_PROGRAM) {
            g->current_pattern = (uint8_t)(e.data1 & 7u);
        }
        return 1u;
    }
    return 0u;
}
