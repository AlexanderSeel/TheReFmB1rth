// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/drum_machine.h"
int main(void){
    static const int16_t pcm[]={12000,8000,4000,0,-4000,-8000,-12000,0};
    drum_machine_t a,b;
    drum_machine_init(&a,DRUM_MODEL_808);drum_machine_init(&b,DRUM_MODEL_909);
    drum_machine_trigger(&a,DRUM_BD,127);drum_machine_trigger(&b,DRUM_SD,100);
    int nz=0;for(int i=0;i<44100;i++)if(drum_machine_process(&a)||drum_machine_process(&b))nz=1;assert(nz);
    drum_machine_trigger(&a,DRUM_OH,127);for(int i=0;i<10;i++)(void)drum_machine_process(&a);int32_t before=a.voice[DRUM_OH].env;drum_machine_trigger(&a,DRUM_CH,127);assert(a.voice[DRUM_OH].env<before);
    drum_machine_set_sample(&a,DRUM_BD,pcm,8u,22050u);assert(drum_machine_sample_available(&a,DRUM_BD));assert(a.sample_use_mask&(1u<<DRUM_BD));
    drum_machine_enable_samples(&a,1u);assert(drum_machine_sample_active_mask(&a)&(1u<<DRUM_BD));
    drum_machine_set_sample_lane(&a,DRUM_BD,0u);assert(!(drum_machine_sample_active_mask(&a)&(1u<<DRUM_BD)));drum_machine_trigger(&a,DRUM_BD,127);assert(a.voice[DRUM_BD].active);assert(!a.sample_voice[DRUM_BD].active);
    drum_machine_set_sample_lane(&a,DRUM_BD,1u);drum_machine_trigger(&a,DRUM_BD,127);assert(a.sample_voice[DRUM_BD].active);assert(!a.voice[DRUM_BD].active);nz=0;for(int i=0;i<32;i++)if(drum_machine_process(&a))nz=1;assert(nz);assert(!a.sample_voice[DRUM_BD].active);
    drum_machine_trigger(&a,DRUM_SD,127);assert(a.voice[DRUM_SD].active);assert(!a.sample_voice[DRUM_SD].active);
    drum_machine_set_sample_use_mask(&a,0u);assert(drum_machine_sample_active_mask(&a)==0u);drum_machine_set_sample_use_mask(&a,0x7ffu);assert(drum_machine_sample_active_mask(&a)==(1u<<DRUM_BD));
    drum_machine_clear_samples(&a);assert(!drum_machine_sample_available(&a,DRUM_BD));assert(a.sample_use_mask==0u);
    puts("drum machine tests: ok");return 0;
}
