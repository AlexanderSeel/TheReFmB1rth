// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/proto/drum_machine.h"

static uint32_t phase_hz(uint32_t inc){return (uint32_t)(((uint64_t)inc*44100ull+(1ull<<31))>>32);}

int main(void){
    static const int16_t pcm[]={12000,8000,4000,0,-4000,-8000,-12000,0};
    drum_machine_t a,b;
    drum_machine_init(&a,DRUM_MODEL_808);drum_machine_init(&b,DRUM_MODEL_909);

    /* The old synthetic fallback started the kick around 390 Hz with a ~23 ms
       decay. Keep the analogue-style body explicitly in bass-drum territory. */
    drum_machine_trigger(&a,DRUM_BD,127);
    drum_machine_trigger(&b,DRUM_BD,127);
    assert(phase_hz(a.voice[DRUM_BD].inc1)>=50u && phase_hz(a.voice[DRUM_BD].inc1)<=60u);
    assert(phase_hz(b.voice[DRUM_BD].inc1)>=55u && phase_hz(b.voice[DRUM_BD].inc1)<=65u);
    assert(a.voice[DRUM_BD].decay_shift>=13u);
    int32_t env0=a.voice[DRUM_BD].env;
    int32_t peak808=0;
    for(int i=0;i<4096;i++){int32_t v=drum_machine_process(&a);if(v<0)v=-v;if(v>peak808)peak808=v;}
    assert(peak808>2500);
    assert(a.voice[DRUM_BD].env>env0/2); /* body survives far beyond a click */

    drum_machine_init(&b,DRUM_MODEL_909);
    drum_machine_trigger(&b,DRUM_SD,127);
    assert(phase_hz(b.voice[DRUM_SD].inc1)>=180u && phase_hz(b.voice[DRUM_SD].inc1)<=220u);
    int32_t peak909=0;
    for(int i=0;i<4096;i++){int32_t v=drum_machine_process(&b);if(v<0)v=-v;if(v>peak909)peak909=v;}
    assert(peak909>2000);

    /* Hats must be actual bipolar metallic audio. The previous signed bitwise
       XOR collapsed oscillator pairs into digital artefacts. */
    drum_machine_init(&a,DRUM_MODEL_808);
    drum_machine_trigger(&a,DRUM_CH,127);
    int pos=0,neg=0;int32_t hat_peak=0;
    for(int i=0;i<1024;i++){int32_t v=drum_machine_process(&a);if(v>100)pos=1;if(v<-100)neg=1;if(v<0)v=-v;if(v>hat_peak)hat_peak=v;}
    assert(pos&&neg&&hat_peak>1000);

    /* A normal kick+snare+hat step retains headroom inside the drum machine. */
    drum_machine_init(&a,DRUM_MODEL_808);
    drum_machine_trigger(&a,DRUM_BD,127);drum_machine_trigger(&a,DRUM_SD,127);drum_machine_trigger(&a,DRUM_CH,127);
    int rails=0;int32_t kit_peak=0;
    for(int i=0;i<4096;i++){int32_t v=drum_machine_process(&a);if(v==32767||v==-32768)rails++;int32_t av=v<0?-v:v;if(av>kit_peak)kit_peak=av;}
    assert(kit_peak>3000);assert(rails==0);

    drum_machine_trigger(&a,DRUM_OH,127);for(int i=0;i<10;i++)(void)drum_machine_process(&a);int32_t before=a.voice[DRUM_OH].env;drum_machine_trigger(&a,DRUM_CH,127);assert(a.voice[DRUM_OH].env<before);
    drum_machine_set_sample(&a,DRUM_BD,pcm,8u,22050u);assert(drum_machine_sample_available(&a,DRUM_BD));assert(a.sample_use_mask&(1u<<DRUM_BD));
    drum_machine_enable_samples(&a,1u);assert(drum_machine_sample_active_mask(&a)&(1u<<DRUM_BD));
    drum_machine_set_sample_lane(&a,DRUM_BD,0u);assert(!(drum_machine_sample_active_mask(&a)&(1u<<DRUM_BD)));drum_machine_trigger(&a,DRUM_BD,127);assert(a.voice[DRUM_BD].active);assert(!a.sample_voice[DRUM_BD].active);
    drum_machine_set_sample_lane(&a,DRUM_BD,1u);drum_machine_trigger(&a,DRUM_BD,127);assert(a.sample_voice[DRUM_BD].active);assert(!a.voice[DRUM_BD].active);int nz=0;for(int i=0;i<32;i++)if(drum_machine_process(&a))nz=1;assert(nz);assert(!a.sample_voice[DRUM_BD].active);
    drum_machine_trigger(&a,DRUM_SD,127);assert(a.voice[DRUM_SD].active);assert(!a.sample_voice[DRUM_SD].active);
    drum_machine_set_sample_use_mask(&a,0u);assert(drum_machine_sample_active_mask(&a)==0u);drum_machine_set_sample_use_mask(&a,0x7ffu);assert(drum_machine_sample_active_mask(&a)==(1u<<DRUM_BD));
    drum_machine_clear_samples(&a);assert(!drum_machine_sample_available(&a,DRUM_BD));assert(a.sample_use_mask==0u);
    puts("drum machine tests: ok");return 0;
}
