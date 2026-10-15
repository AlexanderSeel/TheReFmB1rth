// SPDX-License-Identifier: GPL-3.0-only
#include "drum_machine.h"
#include <string.h>

#define DRUM_SR 44100u

static int16_t clip16(int32_t v){return(int16_t)(v<-32768?-32768:v>32767?32767:v);}
static int32_t tri(uint32_t p){uint32_t q=p>>16;return q<32768u?(int32_t)q*2-32768:98302-(int32_t)q*2;}
static int32_t sq(uint32_t p){return(p&0x80000000u)?-32768:32767;}
static int32_t sineish(uint32_t p){int32_t t=tri(p);int32_t a=t<0?-t:t;return(int32_t)(((int64_t)t*(49152-(a>>1)))>>15);}
static int32_t nz(drum_machine_t*d){uint32_t x=d->noise?d->noise:0x12345678u;x^=x<<13;x^=x>>17;x^=x<<5;d->noise=x;return(int32_t)(x>>16)-32768;}

/* Q32 phase increments at 44.1 kHz. These are intentionally voice/model
 * specific rather than applying a blanket pitch multiplier to the 909. The old
 * fallback placed the 808 kick around 390 Hz and the toms in the several-
 * hundred-Hz range, which made the entire kit sound like short metallic clicks.
 * The values below establish musically useful analogue-drum fundamentals while
 * keeping samples optional on the FM-1 target. */
static const uint32_t inc808[DRUM_VOICES]={
    5356535u,   /* BD  55 Hz */
    17530479u,  /* SD 180 Hz tone + noise */
    116869859u, /* CP 1.2 kHz burst clock */
    48695774u,  /* RS 500 Hz */
    584349292u, /* CH 6.0 kHz metallic pair */
    584349292u, /* OH 6.0 kHz metallic pair */
    8765239u,   /* LT  90 Hz */
    12660901u,  /* MT 130 Hz */
    17530479u,  /* HT 180 Hz */
    389566195u, /* CR 4.0 kHz base */
    292174646u  /* RD 3.0 kHz base */
};
static const uint32_t inc909[DRUM_VOICES]={
    5843493u,   /* BD  60 Hz */
    19478310u,  /* SD 200 Hz */
    126609014u, /* CP 1.3 kHz */
    58434929u,  /* RS 600 Hz */
    633045066u, /* CH 6.5 kHz */
    633045066u, /* OH 6.5 kHz */
    9739155u,   /* LT 100 Hz */
    14171755u,  /* MT ~145 Hz */
    19478310u,  /* HT 200 Hz */
    438261969u, /* CR 4.5 kHz */
    340870420u  /* RD 3.5 kHz */
};

/* Exponential envelope time constants are approximately 2^shift samples.
 * Long tonal decays are essential to kicks/toms/cymbals; hats stay short. */
static const uint8_t dec808[DRUM_VOICES]={14u,13u,12u,11u,9u,12u,13u,13u,13u,14u,15u};
static const uint8_t dec909[DRUM_VOICES]={13u,12u,12u,11u,9u,12u,12u,12u,12u,14u,15u};

static void setup(drum_machine_t*d,drum_voice_id_t id,uint8_t vel){
    const uint32_t*inc=d->model==DRUM_MODEL_909?inc909:inc808;
    const uint8_t*dec=d->model==DRUM_MODEL_909?dec909:dec808;
    drum_voice_state_t*v=&d->voice[id];
    memset(v,0,sizeof(*v));
    v->inc1=inc[id];
    if(id==DRUM_SD) v->inc2=v->inc1+(d->model==DRUM_MODEL_909?12660901u:10713070u); /* second shell mode */
    else if(id==DRUM_CH||id==DRUM_OH) v->inc2=v->inc1+97391549u; /* +1 kHz ring oscillator */
    else if(id==DRUM_CR||id==DRUM_RD) v->inc2=v->inc1+126609014u; /* cymbal partial */
    else v->inc2=v->inc1+4869577u;
    /* ~-2.5 dB below full Q15 at velocity 127 leaves transient and mix headroom. */
    v->env=(int32_t)vel*192;
    v->tone=v->env;
    v->decay_shift=dec[id];
    v->active=1u;
}

void drum_machine_clear_samples(drum_machine_t*d){d->sample_mask=0u;d->sample_use_mask=0u;for(unsigned i=0;i<DRUM_VOICES;i++){d->sample_pcm[i]=0;d->sample_frames[i]=0u;d->sample_rate[i]=44100u;sample_voice_init(&d->sample_voice[i]);}}
void drum_machine_set_sample(drum_machine_t*d,drum_voice_id_t id,const int16_t*pcm,uint32_t frames,uint32_t rate){if((unsigned)id>=DRUM_VOICES)return;d->sample_pcm[id]=pcm;d->sample_frames[id]=frames;d->sample_rate[id]=rate?rate:44100u;if(pcm&&frames){d->sample_mask|=(uint16_t)(1u<<id);d->sample_use_mask|=(uint16_t)(1u<<id);}else{d->sample_mask&=(uint16_t)~(1u<<id);d->sample_use_mask&=(uint16_t)~(1u<<id);}sample_voice_init(&d->sample_voice[id]);}
void drum_machine_enable_samples(drum_machine_t*d,uint8_t enabled){d->samples_enabled=enabled?1u:0u;}
void drum_machine_set_sample_lane(drum_machine_t*d,drum_voice_id_t id,uint8_t enabled){if((unsigned)id>=DRUM_VOICES)return;if(enabled)d->sample_use_mask|=(uint16_t)(1u<<id);else d->sample_use_mask&=(uint16_t)~(1u<<id);}
void drum_machine_set_sample_use_mask(drum_machine_t*d,uint16_t mask){d->sample_use_mask=mask&(uint16_t)((1u<<DRUM_VOICES)-1u);}
uint16_t drum_machine_sample_active_mask(const drum_machine_t*d){return d->samples_enabled?(uint16_t)(d->sample_use_mask&d->sample_mask):0u;}
uint8_t drum_machine_sample_available(const drum_machine_t*d,drum_voice_id_t id){if((unsigned)id>=DRUM_VOICES)return 0u;return(d->sample_mask&(uint16_t)(1u<<id))?1u:0u;}
void drum_machine_init(drum_machine_t*d,drum_model_t model){memset(d,0,sizeof(*d));d->model=model;d->noise=0x13579bdfu;d->samples_enabled=0u;drum_machine_clear_samples(d);}
void drum_machine_trigger(drum_machine_t*d,drum_voice_id_t id,uint8_t vel){uint32_t step;uint16_t bit;if((unsigned)id>=DRUM_VOICES)return;if(!vel)vel=1u;if(id==DRUM_CH){d->voice[DRUM_OH].env>>=3;d->sample_voice[DRUM_OH].active=0u;}bit=(uint16_t)(1u<<id);if(d->samples_enabled&&(d->sample_use_mask&bit)&&drum_machine_sample_available(d,id)){step=(uint32_t)(((uint64_t)d->sample_rate[id]<<16)/DRUM_SR);sample_voice_trigger_rate(&d->sample_voice[id],d->sample_pcm[id],d->sample_frames[id],vel,step);d->voice[id].active=0u;}else setup(d,id,vel);}

static int32_t render(drum_machine_t*d,drum_voice_id_t id){
    drum_voice_state_t*v=&d->voice[id];
    int32_t e,y=0;
    if(!v->active||v->env<=0)return 0;
    v->phase1+=v->inc1;
    v->phase2+=v->inc2;
    e=v->env;
    switch(id){
    case DRUM_BD:{
        /* Classic downward pitch sweep: ~55/60 Hz body, roughly double at the
         * start of the transient rather than the old ~400 Hz metallic click. */
        uint32_t pitch_env=(uint32_t)e<<(d->model==DRUM_MODEL_909?9u:8u);
        v->phase1+=pitch_env;
        y=(sineish(v->phase1)*e)>>15;
        if(d->model==DRUM_MODEL_909){int32_t click=nz(d);int32_t ce=e>8192?e-8192:0;y+=(click*ce)>>18;}
        break;
    }
    case DRUM_SD:{
        int32_t shell=(sineish(v->phase1)+sineish(v->phase2))/2;
        int32_t noise=nz(d);
        y=((shell*e)>>16)+((noise*(e*3/4))>>16);
        break;
    }
    case DRUM_CP:{
        /* Sparse noise bursts approximate the repeated analogue clap impulses. */
        uint32_t burst=(v->phase1>>27)&7u;
        int32_t n=nz(d);
        int32_t b=burst<3u?n:n/4;
        y=(b*e)>>16;
        break;
    }
    case DRUM_RS:{
        int32_t ring=(int32_t)(((int64_t)sq(v->phase1)*sq(v->phase2))>>15);
        y=(ring*e)>>16;
        break;
    }
    case DRUM_CH:case DRUM_OH:{
        /* Ring multiplication, not bitwise XOR. XOR on signed PCM samples was
         * producing digital 0/-1 artefacts instead of a metallic oscillator. */
        int32_t ring=(int32_t)(((int64_t)sq(v->phase1)*sq(v->phase2))>>15);
        int32_t metal=(ring*3+nz(d))/4;
        y=(metal*e)>>15;
        break;
    }
    case DRUM_LT:case DRUM_MT:case DRUM_HT:
        y=(sineish(v->phase1)*e)>>15;
        break;
    case DRUM_CR:case DRUM_RD:{
        int32_t ring=(int32_t)(((int64_t)sq(v->phase1)*sq(v->phase2))>>15);
        int32_t metal=(ring+sq(v->phase1)+nz(d))/3;
        y=(metal*e)>>16;
        break;
    }
    default:break;
    }
    {
        int32_t dd=e>>v->decay_shift;
        if(dd<1)dd=1;
        v->env-=dd;
        if(v->env<48){v->env=0;v->active=0u;}
    }
    return y;
}

int16_t drum_machine_process(drum_machine_t*d){
    int32_t s=0;
    for(unsigned i=0;i<DRUM_VOICES;i++){
        s+=render(d,(drum_voice_id_t)i);
        s+=sample_voice_process(&d->sample_voice[i]);
    }
    /* One machine can legitimately have kick/snare/hat on the same step. Keep
     * six dB of summing headroom here instead of hard-clipping the drum bus. */
    s/=2;
    return clip16(s);
}
