export function installRebirthSkin(mod){
  document.querySelectorAll('.acid').forEach((acid,track)=>{
    const strip=document.createElement('div');strip.className='refm-stock-strip';
    const lab=document.createElement('b');lab.textContent='STOCK 303';strip.appendChild(lab);
    const tuneLabel=document.createElement('span');tuneLabel.textContent='TUNE';strip.appendChild(tuneLabel);
    const tune=document.createElement('input');tune.type='range';tune.min='-64';tune.max='63';tune.value='0';tune.className='refm-tune';tune.oninput=()=>mod._refm_wasm_set_acid_tune(track,Number(tune.value));strip.appendChild(tune);
    const badge=document.createElement('span');badge.className='refm-badge';badge.textContent='SAW / SQUARE · 4-STAGE VCF · ACCENT · SLIDE';strip.appendChild(badge);
    acid.querySelector('.acidHead')?.after(strip);
    const lfo=acid.querySelector('.lfoRow');if(lfo){const btn=document.createElement('button');btn.className='refm-ext-toggle';btn.textContent='EXT / MOD';btn.onclick=()=>lfo.classList.toggle('refm-mod-open');strip.appendChild(btn);}
  });
  const side=document.querySelector('.side');if(side){const box=document.createElement('div');box.className='module';box.innerHTML=`<h3>FM-1 FACE</h3><div class="refm-fm1-preview"><div class="refm-mini-screen"><div>A1 P:A 128</div><div>CUT 62 RES 84</div><div>ENV 91 DEC 48</div><div>WAVE SAW ACC</div><div class="steps">${'<i></i>'.repeat(16)}</div></div><div class="hint">240×240 device skin uses the same hierarchy: machine/pattern/BPM header, 4 macro controls, waveform/accent state and 16-step LEDs. MOD lives on a separate page.</div></div>`;side.appendChild(box);}
}
