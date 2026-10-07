#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/groovebox_midi.h"
#include "../firmware/proto/groovebox_pattern.h"
int main(void){groovebox_t g;groovebox_init(&g,128u);pattern_bank_t b;pattern_bank_init(&b);g.acid_seq[0].step[0].note=42u;g.acid_seq[0].step[0].flags=SEQ16_GATE;groovebox_set_drum_step(&g,0u,0u,1u<<DRUM_BD,0u);groovebox_pattern_store(&b,&g,0u);b.acid[0][1][0].note=55u;b.acid[0][1][0].flags=SEQ16_GATE;groovebox_pattern_load(&b,&g,1u);assert(g.current_pattern==1u&&g.acid_seq[0].step[0].note==55u);midi_router_t r;groovebox_midi_init(&r);assert(groovebox_handle_channel_midi(&g,&r,0x90u,60u,120u));assert(g.acid[0].gate);assert(groovebox_handle_channel_midi(&g,&r,0xB0u,74u,100u));assert(g.acid[0].cutoff==25800u);assert(groovebox_handle_channel_midi(&g,&r,0x99u,36u,127u));assert(g.drum[0].voice[DRUM_BD].active);puts("groovebox pattern/midi tests: ok");return 0;}
