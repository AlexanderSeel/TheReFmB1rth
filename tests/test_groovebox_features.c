#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/groovebox_midi.h"
#include "../firmware/proto/groovebox_pattern.h"

int main(void)
{
    groovebox_t g;
    pattern_bank_t b;
    midi_router_t r;

    groovebox_init(&g, 128u);
    pattern_bank_init(&b);
    g.acid_seq[0].step[0].note = 42u;
    g.acid_seq[0].step[0].flags = SEQ16_GATE;
    groovebox_set_drum_step(&g, 0u, 0u, 1u << DRUM_BD, 0u);
    groovebox_pattern_store(&b, &g, 0u);
    b.acid[0][1][0].note = 55u;
    b.acid[0][1][0].flags = SEQ16_GATE;
    groovebox_pattern_load(&b, &g, 1u);
    assert(g.current_pattern == 1u && g.acid_seq[0].step[0].note == 55u);

    groovebox_midi_init(&r);
    assert(groovebox_handle_channel_midi(&g, &r, 0x90u, 60u, 120u));
    assert(g.acid[0].gate);

    /* Physical/live chromatic notes must reach the 303 unchanged. The previous
       live-MIDI path silently clamped every key above C4/MIDI60 to C4. */
    assert(groovebox_handle_channel_midi(&g, &r, 0x80u, 60u, 0u));
    assert(groovebox_handle_channel_midi(&g, &r, 0x90u, 67u, 120u));
    assert(g.acid[0].phase_inc == acid303_note_to_phase_inc(67u));

    assert(groovebox_handle_channel_midi(&g, &r, 0xB0u, 74u, 100u));
    assert(g.acid[0].cutoff == 25800u);
    assert(groovebox_handle_channel_midi(&g, &r, 0x99u, 36u, 127u));
    assert(g.drum[0].voice[DRUM_BD].active);

    /* Browser/editor pattern-write protocol: select step, then update fields. */
    assert(groovebox_handle_channel_midi(&g, &r, 0xB0u, 102u, 5u));
    assert(r.editor_step == 5u);
    assert(groovebox_handle_channel_midi(&g, &r, 0xB0u, 103u, 84u));
    assert(groovebox_handle_channel_midi(&g, &r, 0xB0u, 104u, 1u));
    assert(groovebox_handle_channel_midi(&g, &r, 0xB0u, 105u, 3u));
    assert(groovebox_handle_channel_midi(&g, &r, 0xB0u, 106u, 87u));
    assert(g.acid_seq[0].step[5].note == 84u);
    assert((g.acid_seq[0].step[5].flags & (SEQ16_GATE | SEQ16_ACCENT | SEQ16_SLIDE)) ==
           (SEQ16_GATE | SEQ16_ACCENT | SEQ16_SLIDE));
    assert(g.acid_seq[0].step[5].probability == 87u);
    assert(groovebox_handle_channel_midi(&g, &r, 0xB0u, 104u, 2u));
    assert((g.acid_seq[0].step[5].flags & SEQ16_TIE) != 0u);
    assert((g.acid_seq[0].step[5].flags & SEQ16_GATE) == 0u);

    /* Drum write protocol uses the same step cursor plus voice/state. */
    assert(groovebox_handle_channel_midi(&g, &r, 0xB2u, 102u, 3u));
    assert(groovebox_handle_channel_midi(&g, &r, 0xB2u, 104u, DRUM_SD));
    assert(groovebox_handle_channel_midi(&g, &r, 0xB2u, 105u, 2u));
    assert((g.drum_hits[0][3] & (1u << DRUM_SD)) != 0u);
    assert((g.drum_accents[0][3] & (1u << DRUM_SD)) != 0u);

    /* Song changes are quantized to the 16-step boundary and load A-H bank. */
    groovebox_init(&g, 128u);
    g.acid_seq[0].step[0].note = 40u;
    g.acid_seq[0].step[0].flags = SEQ16_GATE;
    g.patterns.acid[0][2][0].note = 64u;
    g.patterns.acid[0][2][0].flags = SEQ16_GATE;
    assert(song_append(&g.song, 2u, 1u));
    for (unsigned i = 0; i < 15u; i++) {
        groovebox_advance_step(&g);
        assert(g.current_pattern == 0u);
    }
    groovebox_advance_step(&g);
    assert(g.current_pattern == 2u);
    assert(g.acid_seq[0].step[0].note == 64u);
    assert(g.patterns.acid[0][0][0].note == 40u);

    puts("groovebox pattern/midi tests: ok");
    return 0;
}
