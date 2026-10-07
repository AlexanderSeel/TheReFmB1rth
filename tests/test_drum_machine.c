// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/drum_machine.h"
int main(void){drum_machine_t a,b;drum_machine_init(&a,DRUM_MODEL_808);drum_machine_init(&b,DRUM_MODEL_909);drum_machine_trigger(&a,DRUM_BD,127);drum_machine_trigger(&b,DRUM_SD,100);int nz=0;for(int i=0;i<44100;i++)if(drum_machine_process(&a)||drum_machine_process(&b))nz=1;assert(nz);drum_machine_trigger(&a,DRUM_OH,127);for(int i=0;i<10;i++)(void)drum_machine_process(&a);int32_t before=a.voice[DRUM_OH].env;drum_machine_trigger(&a,DRUM_CH,127);assert(a.voice[DRUM_OH].env<before);puts("drum machine tests: ok");return 0;}
