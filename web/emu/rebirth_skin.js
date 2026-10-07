export function installRebirthSkin(mod){
  const setSlider=(track,cc,value)=>{const el=document.querySelector(`[data-ch="${track}"][data-cc="${cc}"]`);if(el){el.value=String(value);el.dispatchEvent(new Event('input',{bubbles:true}));}};
  const setAccent=(track,value)=>{const el=document.querySelector(`[data-accent="${track}"]`);if(el){el.value=String(value);el.dispatchEvent(new Event('input',{bubbles:true}));}else mod._refm_wasm_set_acid_accent(track,value);};
  const setWave=(track,square)=>{mod._refm_wasm_set_acid_wave(track,square);document.querySelectorAll(`.waveBtn[data-track="${track}"]`).forEach(b=>b.classList.toggle('on',Number(b.dataset.wave)===square));};
  const setTune=(track,value,input)=>{input.value=String(value);mod._refm_wasm_set_acid_tune(track,value);};
  const presets=[
    ['OPEN',0,86,12,0,48,20,0],
    ['SQUELCH',0,18,94,118,54,78,0],
    ['ACCENT',0,32,78,108,42,122,0],
    ['HOLLOW',1,50,92,72,68,58,0]
  ];
  document.querySelectorAll('.acid').forEach((acid,track)=>{
    const strip=document.createElement('div');strip.className='refm-stock-strip';
    const lab=document.createElement('b');lab.textContent='STOCK 303';strip.appendChild(lab);
    const tuneLabel=document.createElement('span');tuneLabel.textContent='TUNE';strip.appendChild(tuneLabel);
    const tune=document.createElement('input');tune.type='range';tune.min='-64';tune.max='63';tune.value='0';tune.className='refm-tune';tune.oninput=()=>mod._refm_wasm_set_acid_tune(track,Number(tune.value));strip.appendChild(tune);
    const badge=document.createElement('span');badge.className='refm-badge';badge.textContent='SAW / SQUARE · 4-STAGE VCF · ACCENT · SLIDE';strip.appendChild(badge);
    acid.querySelector('.acidHead')?.after(strip);

    const audition=document.createElement('div');audition.className='refm-cal-strip';
    const title=document.createElement('b');title.textContent='CAL';audition.appendChild(title);
    presets.forEach(p=>{const b=document.createElement('button');b.className='refm-cal-btn';b.textContent=p[0];b.title='Set a deterministic TB-303 calibration state; use the keyboard or sequencer to audition it.';b.onclick=()=>{setWave(track,p[1]);setSlider(track,74,p[2]);setSlider(track,71,p[3]);setSlider(track,1,p[4]);setSlider(track,73,p[5]);setAccent(track,p[6]);setTune(track,p[7],tune);audition.querySelectorAll('.refm-cal-btn').forEach(x=>x.classList.toggle('on',x===b));};audition.appendChild(b);});
    const hint=document.createElement('span');hint.className='refm-cal-hint';hint.textContent='fixed audition states';audition.appendChild(hint);
    strip.after(audition);

    const lfo=acid.querySelector('.lfoRow');if(lfo){const btn=document.createElement('button');btn.className='refm-ext-toggle';btn.textContent='EXT / MOD';btn.onclick=()=>lfo.classList.toggle('refm-mod-open');strip.appendChild(btn);}
  });
  const side=document.querySelector('.side');if(side){const box=document.createElement('div');box.className='module';box.innerHTML=`<h3>FM-1 FACE</h3><div class="refm-fm1-preview"><div class="refm-mini-screen"><div>A1 P:A 128</div><div>CUT 62 RES 84</div><div>ENV 91 DEC 48</div><div>WAVE SAW ACC</div><div class="steps">${'<i></i>'.repeat(16)}</div></div><div class="hint">240×240 device skin uses the same hierarchy: machine/pattern/BPM header, 4 macro controls, waveform/accent state and 16-step LEDs. MOD lives on a separate page.</div></div>`;side.appendChild(box);}
}
