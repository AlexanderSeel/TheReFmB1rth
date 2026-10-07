// SPDX-License-Identifier: GPL-3.0-only
#include "midi_router.h"
#include "drum_machine.h"
#include <string.h>
void midi_router_init(midi_router_t*r){memset(r,0,sizeof(*r));r->acid1_ch=0u;r->acid2_ch=1u;r->drum808_ch=2u;r->drum909_ch=3u;r->channel10_target=MIDI_TARGET_DRUM808;}static midi_target_t target(const midi_router_t*r,uint8_t ch){if(ch==r->acid1_ch)return MIDI_TARGET_ACID1;if(ch==r->acid2_ch)return MIDI_TARGET_ACID2;if(ch==9u)return(midi_target_t)r->channel10_target;if(ch==r->drum808_ch)return MIDI_TARGET_DRUM808;if(ch==r->drum909_ch)return MIDI_TARGET_DRUM909;return MIDI_TARGET_NONE;}
midi_route_event_t midi_router_decode(const midi_router_t*r,uint8_t st,uint8_t d1,uint8_t d2){midi_route_event_t e;memset(&e,0,sizeof(e));if(st>=0xF0u)return e;uint8_t op=st&0xF0u,ch=st&0x0Fu;e.target=target(r,ch);if(e.target==MIDI_TARGET_NONE)return e;e.data1=d1;e.data2=d2;if(op==0x90u)e.type=d2?MIDI_EVENT_NOTE_ON:MIDI_EVENT_NOTE_OFF;else if(op==0x80u)e.type=MIDI_EVENT_NOTE_OFF;else if(op==0xB0u)e.type=MIDI_EVENT_CC;else if(op==0xC0u)e.type=MIDI_EVENT_PROGRAM;return e;}
int midi_router_drum_voice(uint8_t n){switch(n){case 36:return DRUM_BD;case 38:return DRUM_SD;case 39:return DRUM_CP;case 37:return DRUM_RS;case 42:return DRUM_CH;case 46:return DRUM_OH;case 41:return DRUM_LT;case 45:return DRUM_MT;case 48:return DRUM_HT;case 49:return DRUM_CR;case 51:return DRUM_RD;default:return-1;}}
