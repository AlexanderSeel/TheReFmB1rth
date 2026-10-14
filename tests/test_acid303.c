// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/proto/acid303.h"

static void test_note_order(void) {
    assert(acid303_note_to_phase_inc(60) < acid303_note_to_phase_inc(61));
    assert(acid303_note_to_phase_inc(48) < acid303_note_to_phase_inc(60));
}
static void test_equal_tempered_pitch(void) {
    uint32_t c1=acid303_note_to_phase_inc(24),c2=acid303_note_to_phase_inc(36),c3=acid303_note_to_phase_inc(48),c4=acid303_note_to_phase_inc(60);
    uint32_t a4=acid303_note_to_phase_inc(69);
    assert(c2==c1*2u);assert(c3==c2*2u);assert(c4==c3*2u);
    /* A4=440 Hz at 44.1 kHz => phase increment about 42,852,288. */
    assert(a4>42850000u&&a4<42855000u);
}
static void test_tb303_service_pitch_anchor(void) {
    /* Roland's TB-303 service calibration centers the TUNE control and trims
       the low A key to 110 Hz, then checks the higher register at a 4:1 ratio.
       MIDI A2 is note 45 in this engine. Keep that physical calibration anchor
       explicit in addition to the generic A4=440 equal-temperament test. */
    uint32_t a2=acid303_note_to_phase_inc(45u);
    uint32_t a3=acid303_note_to_phase_inc(57u);
    uint32_t a4=acid303_note_to_phase_inc(69u);
    uint64_t mhz=((uint64_t)a2*44100ull*1000ull+(1ull<<31))>>32;
    assert(mhz>=109950ull&&mhz<=110050ull);
    assert(a3==a2*2u);
    assert(a4==a2*4u);
}

/* This deliberately measures the final acid303_process() waveform, rather than
   trusting only the DDS increment. A pitch-table bug can therefore not remain
   hidden behind otherwise-green arithmetic tests. With a square source,
   resonance off and the cutoff fully open, the conditioned 303 output must
   retain one positive zero crossing per oscillator period. */
static uint32_t rendered_positive_crossings(uint8_t note) {
    acid303_t s;
    int32_t prev;
    uint32_t count=0u;
    acid303_init(&s);
    s.square=1u;
    s.cutoff=32767u;
    s.resonance=0u;
    s.env_mod=0u;
    s.accent=0u;
    s.drive=0u;
    s.tune=0;
    acid303_set_note(&s,note,0u,0u);
    for(uint32_t i=0u;i<4096u;++i)(void)acid303_process(&s);
    prev=acid303_process(&s);
    for(uint32_t i=0u;i<22050u;++i){
        int32_t y=acid303_process(&s);
        if(prev<=0&&y>0)++count;
        prev=y;
    }
    return count;
}
static void test_rendered_pitch_end_to_end(void) {
    uint32_t c2=rendered_positive_crossings(36u);
    uint32_t c3=rendered_positive_crossings(48u);
    uint32_t c4=rendered_positive_crossings(60u);
    printf("rendered 303 pitch crossings/0.5s: C2=%u C3=%u C4=%u\n",c2,c3,c4);
    /* Expected counts for 65.4, 130.8 and 261.6 Hz over half a second.
       Allow margin for the analog-conditioning/filter path around zero. */
    assert(c2>=29u&&c2<=37u);
    assert(c3>=61u&&c3<=70u);
    assert(c4>=126u&&c4<=136u);
    assert(c3>c2*18u/10u&&c3<c2*22u/10u);
    assert(c4>c3*18u/10u&&c4<c3*22u/10u);
}
static void test_tune_control(void) {
    uint32_t base = acid303_note_to_phase_inc(48);
    assert(acid303_apply_tune(base, -12) == base/2u);
    assert(acid303_apply_tune(base, 0) == base);
    assert(acid303_apply_tune(base, 12) == base*2u);
    assert(acid303_apply_tune(base, -64) == acid303_apply_tune(base, -12));
    assert(acid303_apply_tune(base, 63) == acid303_apply_tune(base, 12));
    acid303_t a,b; acid303_init(&a); acid303_init(&b); a.tune=-12; b.tune=12;
    acid303_set_note(&a,48,0,0); acid303_set_note(&b,48,0,0);
    assert(a.phase_inc < base && b.phase_inc > base);
}
static void test_output_is_bounded(void) { acid303_t s; acid303_init(&s); acid303_set_note(&s,48,1,0); s.lfo_amount=127u; for(int i=0;i<44100*2;++i){int32_t y=acid303_process(&s);assert(y>=-32768&&y<=32767);} }
static void test_aggressive_patch_does_not_live_on_the_rails(void){
    acid303_t s;acid303_init(&s);s.square=0u;s.cutoff=30000u;s.resonance=30000u;s.env_mod=30000u;s.accent=32767u;s.drive=0u;
    acid303_set_note(&s,36u,1u,0u);
    uint32_t rails=0u;int32_t peak=0;
    for(uint32_t i=0u;i<22050u;++i){int32_t y=acid303_process(&s);int32_t a=y<0?-y:y;if(a>peak)peak=a;if(y<=-32768||y>=32767)rails++;}
    /* Fixed-point source headroom is allowed to touch a rail on an isolated
       extreme transient, but a sustained hard-clipped waveform is not a 303. */
    assert(peak>2000);assert(rails<220u);
}
static void test_stock_mode_has_no_lfo(void){acid303_t s;acid303_init(&s);assert(s.lfo_amount==0u);uint32_t p=s.lfo_phase;acid303_set_note(&s,48,0,0);for(int i=0;i<4096;i++)acid303_process(&s);assert(s.lfo_phase==p);}
static void test_measured_cutoff_mapping(void) { acid303_t s; acid303_init(&s); s.cutoff=0u; assert(acid303_cutoff_hz(&s)>=310u&&acid303_cutoff_hz(&s)<=318u); s.cutoff=16384u; uint16_t mid=acid303_cutoff_hz(&s); assert(mid>=850u&&mid<=890u); s.cutoff=32767u; assert(acid303_cutoff_hz(&s)>=2385u&&acid303_cutoff_hz(&s)<=2400u); }
static void test_slide_is_legato_and_converges(void) { acid303_t s; acid303_init(&s); acid303_set_note(&s,48,0,0); for(int i=0;i<100;++i)acid303_process(&s); int32_t env_before=s.env; uint32_t start=s.phase_inc; acid303_set_note(&s,60,0,1); uint32_t target=s.slide_target_inc; assert(target>start); assert(s.env==env_before); for(int i=0;i<4000;++i)acid303_process(&s); assert(s.phase_inc>start); assert(s.phase_inc<=target); }
static void test_stock_slide_time_constant(void){acid303_t s;acid303_init(&s);acid303_set_note(&s,48,0,0);for(int i=0;i<64;i++)acid303_process(&s);acid303_set_note(&s,60,0,1);uint32_t target=s.slide_target_inc,start=s.phase_inc;uint32_t d0=target-start;for(int i=0;i<529;i++)acid303_process(&s);uint32_t d1=target-s.phase_inc;assert(d1*100u>d0*32u);assert(d1*100u<d0*42u);}
static void test_slide_updates_accent_without_retrigger(void){acid303_t s;acid303_init(&s);s.accent=32767u;acid303_set_note(&s,48,0,0);for(int i=0;i<200;i++)acid303_process(&s);int32_t env=s.env;acid303_set_note(&s,52,1,1);assert(s.env==env);assert(s.accented==1u);acid303_process(&s);assert(s.accent_env>0&&s.accent_env<s.env);}
static void test_note_off_closes_vca(void) { acid303_t s; acid303_init(&s); acid303_set_note(&s,52,0,0); for(int i=0;i<100;++i)acid303_process(&s); acid303_note_off(&s); for(int i=0;i<20000;++i)acid303_process(&s); assert(s.amp==0); assert(s.amp_stage==ACID_ENV_OFF); assert(s.idle==1u); }
static void test_accent_release_is_longer(void){acid303_t normal,accent;acid303_init(&normal);acid303_init(&accent);acid303_set_note(&normal,48,0,0);acid303_set_note(&accent,48,1,0);for(int i=0;i<100;i++){acid303_process(&normal);acid303_process(&accent);}acid303_note_off(&normal);acid303_note_off(&accent);for(int i=0;i<200;i++){acid303_process(&normal);acid303_process(&accent);}assert(accent.amp>normal.amp);}
static uint64_t articulation_energy(uint8_t accented){acid303_t s;acid303_init(&s);s.cutoff=14000u;s.resonance=18000u;s.env_mod=23000u;s.accent=32767u;acid303_set_note(&s,48,accented,0);uint64_t e=0;for(int i=0;i<2205;i++){int32_t y=acid303_process(&s);e+=(uint32_t)(y<0?-y:y);}return e;}
static void test_accent_vca_is_materially_stronger(void){uint64_t normal=articulation_energy(0u),accent=articulation_energy(1u);assert(accent>normal);}
static void test_idle_only_resets_analog_path(void){acid303_t s;acid303_init(&s);acid303_set_note(&s,48,0,0);for(int i=0;i<50;i++)acid303_process(&s);uint32_t phase=s.phase;assert(phase!=0u);acid303_set_note(&s,52,0,0);assert(s.phase==phase);acid303_note_off(&s);for(int i=0;i<10000;i++)acid303_process(&s);assert(s.idle==1u);acid303_set_note(&s,55,0,0);assert(s.phase==0u);}
static void test_open303_rc_paths(void){acid303_t n,a;acid303_init(&n);acid303_init(&a);a.accent=32767u;acid303_set_note(&n,48,0,0);acid303_set_note(&a,48,1,0);acid303_process(&n);acid303_process(&a);assert(n.env_rc==n.env);assert(n.accent_env==0);assert(a.env_rc==a.env);assert(a.accent_env>0&&a.accent_env<a.env);}
static void test_saw_and_303_square_are_different(void) { acid303_t a,b; acid303_init(&a);acid303_init(&b);a.square=0u;b.square=1u;acid303_set_note(&a,48,0,0);acid303_set_note(&b,48,0,0);int different=0;for(int i=0;i<512;++i){if(acid303_process(&a)!=acid303_process(&b)){different=1;break;}}assert(different); }
static void test_lfo_runs_all_shapes(void) { acid303_t s;acid303_init(&s);s.lfo_amount=90u;int changed=0;for(uint8_t shape=0;shape<4u;++shape){s.lfo_shape=shape;s.lfo_phase=0u;int16_t first=acid303_lfo_value(&s);for(int i=0;i<4096;++i)acid303_process(&s);int16_t later=acid303_lfo_value(&s);if(later!=first)changed++;}assert(changed==4); }
static uint64_t render_shape_hash(uint8_t shape){acid303_t s;acid303_init(&s);s.lfo_shape=shape;s.lfo_amount=110u;s.lfo_rate=90u;acid303_set_note(&s,48,0,0);uint64_t h=1469598103934665603ull;for(int i=0;i<12000;++i){uint16_t y=(uint16_t)acid303_process(&s);h^=(uint8_t)y;h*=1099511628211ull;h^=(uint8_t)(y>>8);h*=1099511628211ull;}return h;}
static void test_lfo_shapes_change_audio(void){uint64_t h[4];for(uint8_t i=0;i<4u;++i)h[i]=render_shape_hash(i);for(unsigned i=0;i<4u;++i)for(unsigned j=i+1u;j<4u;++j)assert(h[i]!=h[j]);}
int main(void){test_note_order();test_equal_tempered_pitch();test_tb303_service_pitch_anchor();test_rendered_pitch_end_to_end();test_tune_control();test_output_is_bounded();test_aggressive_patch_does_not_live_on_the_rails();test_stock_mode_has_no_lfo();test_measured_cutoff_mapping();test_slide_is_legato_and_converges();test_stock_slide_time_constant();test_slide_updates_accent_without_retrigger();test_note_off_closes_vca();test_accent_release_is_longer();test_accent_vca_is_materially_stronger();test_idle_only_resets_analog_path();test_open303_rc_paths();test_saw_and_303_square_are_different();test_lfo_runs_all_shapes();test_lfo_shapes_change_audio();puts("acid303 circuit-model tests: ok");return 0;}
