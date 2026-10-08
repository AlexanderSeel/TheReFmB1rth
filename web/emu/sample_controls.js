// Browser auxiliary controls. No shadow musical state: all truth comes from WASM.
const NAMES=['BD','SD','CP','RS','CH','OH','LT','MT','HT','CR','RD'];
const NOTE_NAMES=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
const GATE=1,ACCENT=2,SLIDE=4,TIE=8;
function bit(mask,lane){return !!(mask&(1<<lane));}
function clamp(v,a,b){return Math.max(a,Math.min(b,v));}
function noteLabel(n){return `${NOTE_NAMES[n%12]}${Math.floor(n/12)-1}`;}

function installAcidTimingControls(mod){
  if(!mod?._refm_wasm_get_acid_step||!mod?._refm_wasm_set_acid_step)return;
  const read=(track,step)=>{const x=mod._refm_wasm_get_acid_step(track,step)>>>0;return{note:x&255,flags:(x>>>8)&255,prob:(x>>>16)&255};};
  const write=(track,step,s)=>mod._refm_wasm_set_acid_step(track,step,clamp(s.note||48,0,127),s.flags&15,clamp(s.prob??100,0,100));
  const selectedStep=track=>{const b=document.querySelector(`#seq${track} .step.selected`);return b?+b.dataset.i:0;};
  const modeOf=s=>(s.flags&TIE)&&!(s.flags&GATE)?'tie':(s.flags&GATE)?'note':'rest';
  const paintStep=(track,step)=>{
    const b=document.querySelector(`#seq${track} .step[data-i="${step}"]`);if(!b)return;
    const s=read(track,step),mode=modeOf(s),note=mode==='note'?noteLabel(s.note):mode==='tie'?'TIE':'REST';
    b.className='step'+(mode==='note'?' on':'')+(s.flags&ACCENT?' accent':'')+(s.flags&SLIDE?' slide':'')+(mode==='tie'?' tie':'')+(b.classList.contains('playing')?' playing':'')+(b.classList.contains('selected')?' selected':'');
    b.innerHTML=`<span class="refm-step-num">${step+1}</span><strong>${note}</strong><small>${mode==='note'&&(s.flags&ACCENT)?'ACC ':''}${mode==='note'&&(s.flags&SLIDE)?'SLD ':''}${s.prob<100?`${s.prob}%`:''}</small>`;
  };
  const refresh=(editor,track)=>{
    const step=selectedStep(track),s=read(track,step),mode=modeOf(s);
    editor.querySelectorAll('[data-time]').forEach(b=>b.classList.toggle('on',b.dataset.time===mode));
    editor.querySelectorAll('[data-modifier]').forEach(b=>{const f=+b.dataset.modifier;b.disabled=mode!=='note';b.classList.toggle('on',mode==='note'&&!!(s.flags&f));});
    const readout=editor.querySelector('.refm-editor-readout em');if(readout)readout.textContent=mode==='note'?noteLabel(s.note):mode.toUpperCase();
    const piano=editor.querySelector('.refm-piano');if(piano)piano.classList.toggle('refm-disabled',mode!=='note');
  };
  document.querySelectorAll('.refm-note-editor').forEach(editor=>{
    const track=+editor.dataset.track,flags=editor.querySelector('.refm-flags');if(!flags)return;
    flags.innerHTML='<div class="refm-time-mode"><span>TIME MODE</span><button data-time="note">NOTE</button><button data-time="rest">REST</button><button data-time="tie">TIE</button></div><div class="refm-note-modifiers"><span>NOTE MOD</span><button data-modifier="2">ACCENT</button><button data-modifier="4">SLIDE</button></div><button data-copy>COPY</button><button data-paste>PASTE</button>';
    let clipboard=null;
    flags.querySelectorAll('[data-time]').forEach(b=>b.onclick=()=>{
      const step=selectedStep(track),s=read(track,step),mode=b.dataset.time;
      if(mode==='note'){s.flags=(s.flags&(ACCENT|SLIDE))|GATE;}
      else if(mode==='tie'){s.flags=TIE;}
      else{s.flags=0;}
      write(track,step,s);paintStep(track,step);refresh(editor,track);
    });
    flags.querySelectorAll('[data-modifier]').forEach(b=>b.onclick=()=>{
      const step=selectedStep(track),s=read(track,step);if(modeOf(s)!=='note')return;
      s.flags^=+b.dataset.modifier;write(track,step,s);paintStep(track,step);refresh(editor,track);
    });
    flags.querySelector('[data-copy]').onclick=()=>{clipboard={...read(track,selectedStep(track))};};
    flags.querySelector('[data-paste]').onclick=()=>{if(!clipboard)return;const step=selectedStep(track);write(track,step,{...clipboard});paintStep(track,step);refresh(editor,track);};
    editor.addEventListener('click',e=>{
      if(e.target.closest('[data-note],[data-oct]'))queueMicrotask(()=>{const step=selectedStep(track),s=read(track,step);s.flags=(s.flags&(ACCENT|SLIDE))|GATE;write(track,step,s);paintStep(track,step);refresh(editor,track);});
      if(e.target.closest('.step'))queueMicrotask(()=>refresh(editor,track));
    });
    document.querySelector(`#seq${track}`)?.addEventListener('click',()=>queueMicrotask(()=>refresh(editor,track)));
    window.addEventListener('refm:patternchange',()=>queueMicrotask(()=>{for(let s=0;s<16;s++)paintStep(track,s);refresh(editor,track);}));
    for(let s=0;s<16;s++)paintStep(track,s);refresh(editor,track);
  });
}

export function installSampleControls(mod){
  installAcidTimingControls(mod);
  if(!mod?._refm_wasm_sample_mask||!mod?._refm_wasm_set_sample_lane)return;
  const refresh=(track,root)=>{
    const avail=mod._refm_wasm_sample_mask(track)>>>0;
    const use=mod._refm_wasm_sample_use_mask(track)>>>0;
    const active=mod._refm_wasm_sample_active_mask(track)>>>0;
    root.querySelectorAll('[data-lane]').forEach(b=>{
      const lane=+b.dataset.lane,a=bit(avail,lane),u=bit(use,lane),on=bit(active,lane);
      b.disabled=!a;b.classList.toggle('sample-active',on);b.classList.toggle('sample-selected',u&&!on);b.classList.toggle('fallback',!a);
      b.textContent=`${NAMES[lane]} · ${on?'SAMPLE':a&&u?'READY':'SYNTH'}`;
      b.title=a?(on?'Bundled sample is active':'Sample available; click to select'):'No bundled sample for this lane; synthesis fallback';
    });
    const c=root.querySelector('[data-count]');if(c)c.textContent=`${NAMES.filter((_,i)=>bit(active,i)).length} sample lanes active`;
    const s=root.closest('[data-refm-drum-track]')?.querySelector('.refm-drum-source-status');if(s)s.textContent=active?'HYBRID':'SYNTH';
  };
  document.querySelectorAll('[data-refm-drum-track]').forEach(host=>{
    const track=+host.dataset.refmDrumTrack;
    const panel=document.createElement('div');panel.className='refm-source-panel';panel.dataset.sourceTrack=track;
    panel.innerHTML=`<div class="refm-source-head"><b>${track?'909':'808'} SOURCES</b><span data-count></span><button class="refm-source-all">HYBRID ALL</button><button class="refm-source-synth">SYNTH ALL</button></div><div class="refm-source-lanes"></div>`;
    const lanes=panel.querySelector('.refm-source-lanes');
    for(let lane=0;lane<NAMES.length;lane++){
      const b=document.createElement('button');b.className='refm-source-lane';b.dataset.lane=lane;
      b.onclick=()=>{const use=mod._refm_wasm_sample_use_mask(track)>>>0;mod._refm_wasm_set_sample_lane(track,lane,bit(use,lane)?0:1);mod._refm_wasm_enable_samples(track,1);refresh(track,panel);};lanes.appendChild(b);
    }
    panel.querySelector('.refm-source-all').onclick=()=>{mod._refm_wasm_enable_samples(track,1);mod._refm_wasm_set_sample_use_mask(track,0x7ff);refresh(track,panel);};
    panel.querySelector('.refm-source-synth').onclick=()=>{mod._refm_wasm_set_sample_use_mask(track,0);refresh(track,panel);};
    host.appendChild(panel);refresh(track,panel);
  });
}
