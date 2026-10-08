// SPDX-License-Identifier: GPL-3.0-only
/*
 * Browser-only high-quality TB-303 voice.
 *
 * This is an independent C adaptation of the MIT-licensed Open303 DSP
 * structure by Robin Schmidt. It intentionally uses floating point and 4x
 * oversampling in WASM so browser fidelity is not constrained by the FM-1
 * fixed-point/CPU budget. The hardware build continues to use
 * firmware/proto/acid303.c.
 */
#include "../../firmware/proto/acid303.h"
#include "../../firmware/proto/acid303_wavetable.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SR 44100.0
#define OS 4.0
#define OSR (SR*OS)
#define PI 3.14159265358979323846
#define HQ_SLOTS 4

typedef struct {
    acid303_t *owner;
    double phase;
    double freq;
    double target_freq;
    double y1,y2,y3,y4;
    double fb_x1,fb_y1;
    double pre_x1,pre_y1;
    double post_x1,post_y1;
    double ap_x1,ap_y1;
    double notch_x1,notch_x2,notch_y1,notch_y2;
    double aa_w[12];
    double declick_x1,declick_x2,declick_y1,declick_y2;
    double main_env;
    double amp_env;
    double accent_rc;
} hq_state_t;

static hq_state_t hq[HQ_SLOTS];

static double dclamp(double x,double lo,double hi){return x<lo?lo:x>hi?hi:x;}
static int32_t iclamp(double x){if(x>32767.0)return 32767;if(x<-32768.0)return -32768;return (int32_t)lrint(x);}

static hq_state_t *st_for(acid303_t *s){
    unsigned i;
    for(i=0;i<HQ_SLOTS;i++)if(hq[i].owner==s)return &hq[i];
    for(i=0;i<HQ_SLOTS;i++)if(!hq[i].owner){memset(&hq[i],0,sizeof(hq[i]));hq[i].owner=s;return &hq[i];}
    hq[0].owner=s;return &hq[0];
}

static void reset_hq(hq_state_t *q){
    acid303_t *o=q->owner;memset(q,0,sizeof(*q));q->owner=o;
}

static const uint32_t semitone_inc[12]={159321,168796,178834,189468,200734,212670,225316,238714,252908,267948,283881,300760};
uint32_t acid303_note_to_phase_inc(uint8_t note){uint32_t octave=note/12u,inc=semitone_inc[note%12u];if(octave){if(octave>=8u)octave=7u;inc<<=octave;}return inc;}
uint32_t acid303_apply_tune(uint32_t inc,int8_t tune){double f=(double)inc*pow(2.0,(double)tune/1536.0);if(f<1.0)f=1.0;if(f>4294967295.0)f=4294967295.0;return(uint32_t)llround(f);}
static double inc_to_hz(uint32_t inc){return(double)inc*SR/4294967296.0;}

uint16_t acid303_cutoff_hz(const acid303_t *s){
    const double c0=313.8152786059267,c1=2394.411986817546;
    double n=dclamp((double)s->cutoff/32767.0,0.0,1.0);
    return(uint16_t)lrint(c0*pow(c1/c0,n));
}

int16_t acid303_lfo_value(const acid303_t *s){
    uint32_t q=s->lfo_phase>>16;double p=(double)q/65536.0,y;
    switch((acid_lfo_shape_t)(s->lfo_shape&3u)){
    case ACID_LFO_TRI:y=p<0.5?(4.0*p-1.0):(3.0-4.0*p);break;
    case ACID_LFO_SAW:y=2.0*p-1.0;break;
    case ACID_LFO_SQUARE:y=p<0.5?1.0:-1.0;break;
    default:y=sin(2.0*PI*p);break;
    }
    return(int16_t)iclamp(y*32767.0);
}

void acid303_init(acid303_t *s){
    hq_state_t *q=st_for(s);memset(s,0,sizeof(*s));reset_hq(q);
    s->cutoff=10500u;s->resonance=19000u;s->env_mod=23500u;s->decay=12500u;s->accent=21000u;s->drive=0u;s->tune=0;
    s->amp_sustain=127u;s->amp_release=110u;s->lfo_rate=35u;s->lfo_amount=0u;s->lfo_shape=ACID_LFO_SINE;s->amp_stage=ACID_ENV_OFF;s->idle=1u;
}

void acid303_set_note(acid303_t *s,uint8_t note,uint8_t accent,uint8_t slide){
    hq_state_t *q=st_for(s);uint32_t inc=acid303_apply_tune(acid303_note_to_phase_inc(note),s->tune);double f=inc_to_hz(inc);
    if(slide&&s->gate){q->target_freq=f;s->sliding=1u;s->accented=accent?1u:0u;}
    else{
        if(s->idle)reset_hq(q);
        q->freq=q->target_freq=f;s->phase_inc=inc;s->slide_target_inc=inc;s->sliding=0u;
        q->main_env=1.0;q->amp_env=1.0;q->accent_rc=0.0;s->env=32767;s->amp=32767;s->amp_stage=ACID_ENV_DECAY;s->accented=accent?1u:0u;
    }
    s->gate=1u;s->idle=0u;
}
void acid303_note_off(acid303_t *s){s->gate=0u;s->amp_stage=ACID_ENV_RELEASE;}

static double hp(double in,double cutoff,double fs,double *x1,double *y1){
    double x=exp(-2.0*PI*cutoff/fs),b0=0.5*(1.0+x),y=b0*in-b0*(*x1)+x*(*y1);*x1=in;*y1=y;return y;
}
static double allpass(double in,double cutoff,double fs,double *x1,double *y1){
    double t=tan(PI*cutoff/fs),x=(t-1.0)/(t+1.0),y=x*in+(*x1)-x*(*y1);*x1=in;*y1=y;return y;
}

static double notch(hq_state_t*q,double in){
    const double f=7.5164,bw=4.7,w=2.0*PI*f/SR,s=sin(w),c=cos(w);
    double alpha=s*sinh(0.5*log(2.0)*bw*w/s),scale=1.0/(1.0+alpha);
    double a1=2.0*c*scale,a2=(alpha-1.0)*scale,b0=scale,b1=-2.0*c*scale,b2=scale;
    double y=b0*in+b1*q->notch_x1+b2*q->notch_x2+a1*q->notch_y1+a2*q->notch_y2;
    q->notch_x2=q->notch_x1;q->notch_x1=in;q->notch_y2=q->notch_y1;q->notch_y1=y;return y;
}

static double declick(hq_state_t*q,double in){
    const double f=200.0,w=2.0*PI*f/SR,s=sin(w),c=cos(w),Q=sqrt(0.5),alpha=s/(2.0*Q),scale=1.0/(1.0+alpha);
    double a1=2.0*c*scale,a2=(alpha-1.0)*scale,b1=(1.0-c)*scale,b0=0.5*b1,b2=b0;
    double y=b0*in+b1*q->declick_x1+b2*q->declick_x2+a1*q->declick_y1+a2*q->declick_y2;
    q->declick_x2=q->declick_x1;q->declick_x1=in;q->declick_y2=q->declick_y1;q->declick_y1=y;return y;
}

static double antialias(hq_state_t*q,double in){
    static const double a[12]={-9.1891604652189471,40.177553696870497,-110.11636661771178,210.18506612078195,-293.84744771903240,308.16345558359234,-244.06786780384243,144.81877911392738,-62.770692151724198,18.867762095902137,-3.5327094230551848,0.31183189275203149};
    static const double b[13]={0.00013671732099945628,-0.00055538501265606384,0.0013681887636296387,-0.0022158566490711852,0.0028320091007278322,-0.0029776933151090413,0.0030283628243514991,-0.0029776933151090413,0.0028320091007278331,-0.0022158566490711861,0.0013681887636296393,-0.00055538501265606384,0.00013671732099945636};
    double tmp=in,y;int i;for(i=0;i<12;i++)tmp-=a[i]*q->aa_w[i];y=b[0]*tmp;for(i=0;i<12;i++)y+=b[i+1]*q->aa_w[i];for(i=11;i>0;i--)q->aa_w[i]=q->aa_w[i-1];q->aa_w[0]=tmp;return y;
}

static double osc(hq_state_t*q,acid303_t*s){
    uint32_t inc=(uint32_t)llround(q->freq/SR*4294967296.0),band=0,idx;double frac,a,b;
    while(band+1u<REFM_ACID_WT_BANDS&&inc>refm_acid_wt_threshold_inc[band])++band;
    q->phase+=q->freq/OSR;if(q->phase>=1.0)q->phase-=floor(q->phase);
    {
        const int16_t *tab=s->square?refm_acid_wt_square[band]:refm_acid_wt_saw[band];
        double p=q->phase*(double)REFM_ACID_WT_SIZE;idx=(uint32_t)p&REFM_ACID_WT_MASK;frac=p-floor(p);a=tab[idx];b=tab[(idx+1u)&REFM_ACID_WT_MASK];
        return(a+(b-a)*frac)/32768.0;
    }
}

static void env_and_pitch(hq_state_t*q,acid303_t*s){
    double glideA=1.0-exp(-1.0/(0.012*SR));
    if(s->sliding){q->freq+=glideA*(q->target_freq-q->freq);if(fabs(q->target_freq-q->freq)<0.001){q->freq=q->target_freq;s->sliding=0u;}}
    {
        double normalMs=200.0+1800.0*dclamp((double)s->decay/32767.0,0.0,1.0),tau=s->accented?200.0:normalMs;
        q->main_env*=exp(-1.0/(0.001*tau*SR));
    }
    {
        double target=(s->accented&&s->accent)?q->main_env:0.0,a=1.0-exp(-1.0/(0.015*SR));q->accent_rc+=a*(target-q->accent_rc);
    }
    {
        double tau=s->gate?1230.0:(s->accented?50.0:1.0);q->amp_env*=exp(-1.0/(0.001*tau*SR));
        if(!s->gate&&q->amp_env<1e-7){q->amp_env=0.0;s->idle=1u;s->amp_stage=ACID_ENV_OFF;}
    }
    s->env=(int32_t)iclamp(q->main_env*32767.0);s->amp=(int32_t)iclamp(q->amp_env*32767.0);
}

static double inst_cutoff(acid303_t*s,hq_state_t*q){
    const double c0=313.8152786059267,c1=2394.411986817546,oF=0.048292930943553,oC=0.294391201442418,sLoF=3.773996325111173,sLoC=0.736965594166206,sHiF=4.194548788411135,sHiC=0.864344900642434;
    double cutoff=(double)acid303_cutoff_hz(s),e=dclamp((double)s->env_mod/32767.0,0.0,1.0),c=log(cutoff/c0)/log(c1/c0),sLo=sLoF*e+sLoC,sHi=sHiF*e+sHiC,sc=(1.0-c)*sLo+c*sHi,off=oF*c+oC;
    double tmp1=sc*(q->main_env-off),tmp2=dclamp((double)s->accent/32767.0,0.0,1.0)*q->accent_rc;
    double fc=cutoff*pow(2.0,tmp1+tmp2);
    if(s->lfo_amount){double l=(double)acid303_lfo_value(s)/32768.0;fc+=l*((double)s->lfo_amount/127.0)*1600.0;}
    return dclamp(fc,200.0,20000.0);
}

static double ladder(hq_state_t*q,acid303_t*s,double in,double fc){
    double raw=dclamp((double)s->resonance/32767.0,0.0,1.0),r=(1.0-exp(-3.0*raw))/(1.0-exp(-3.0));
    double fx=fc/(OSR*sqrt(2.0));
    double b0=(0.00045522346+6.1922189*fx)/(1.0+12.358354*fx+4.4156345*fx*fx);
    double k=fx*(fx*(fx*(fx*(fx*(fx+7198.6997)-5837.7917)-476.47308)+614.95611)+213.87126)+16.998792;
    double g=k/17.0;g=(g-1.0)*r+1.0;g*=1.0+r;k*=r;
    double fb=hp(k*q->y4,150.0,OSR,&q->fb_x1,&q->fb_y1),y0=in-fb;
    q->y1+=2.0*b0*(y0-q->y1+q->y2);q->y2+=b0*(q->y1-2.0*q->y2+q->y3);q->y3+=b0*(q->y2-2.0*q->y3+q->y4);q->y4+=b0*(q->y3-2.0*q->y4);
    return 2.0*g*q->y4;
}

int32_t acid303_process(acid303_t*s){
    hq_state_t*q=st_for(s);int i;double fc,tmp=0.0,gain,out;
    if(s->idle)return 0;
    if(s->lfo_amount)s->lfo_phase+=4870u+(uint32_t)s->lfo_rate*15310u;
    env_and_pitch(q,s);fc=inst_cutoff(s,q);
    for(i=0;i<4;i++){
        double x=-osc(q,s);x=hp(x,44.486,OSR,&q->pre_x1,&q->pre_y1);tmp=ladder(q,s,x,fc);tmp=antialias(q,tmp);
    }
    tmp=allpass(tmp,14.008,SR,&q->ap_x1,&q->ap_y1);tmp=hp(tmp,24.167,SR,&q->post_x1,&q->post_y1);tmp=notch(q,tmp);
    gain=q->amp_env;if(s->gate)gain+=0.45*q->main_env+(s->accented?4.0*dclamp((double)s->accent/32767.0,0.0,1.0)*q->main_env:0.0);gain=declick(q,gain);
    out=tmp*gain;
    if(s->drive){double d=(double)s->drive/2048.0;out=(d*out)/(1.0+d*fabs(out));}
    out*=0.251188643150958; /* Open303 default postgain -12 dB. */
    return iclamp(out*32767.0);
}
