#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/midi_router.h"
int main(void){midi_router_t r;midi_router_init(&r);midi_route_event_t e=midi_router_decode(&r,0x90u,60u,100u);assert(e.type==MIDI_EVENT_NOTE_ON&&e.target==MIDI_TARGET_ACID1);e=midi_router_decode(&r,0x91u,61u,0u);assert(e.type==MIDI_EVENT_NOTE_OFF&&e.target==MIDI_TARGET_ACID2);e=midi_router_decode(&r,0x99u,36u,127u);assert(e.target==MIDI_TARGET_DRUM808);assert(midi_router_drum_voice(36u)==0);assert(midi_router_drum_voice(51u)>=0);assert(midi_router_drum_voice(60u)<0);puts("midi router tests: ok");return 0;}
