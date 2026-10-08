const DRUM_NAMES=['BD','SD','CP','RS','CH','OH','LT','MT','HT','CR','RD'];
const TRACK_NAMES=['ACID A','ACID B','DRUM 808','DRUM 909'];
const NOTE_NAMES=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));

export function installRebirthSkin(mod){
  document.body.classList.add('refm-rack-mode');
  const rack=document.querySelector('.rack'); if(!rack)return;
  rack.classList.add('refm-console');

  const midi=(...bytes)=>bytes.forEach(b=>mod._refm_wasm_midi(b));
  const noteOn=(track,n)=>midi(0x90|track,n,112), noteOff=(track,n)=>midi(0x80|track,n,0);
  const readStep=(track,step)=>{const x=mod._refm_wasm_get_acid_step(track,step)>>>0;return{note:x&255,flags:(x>>>8)&255,prob:(x>>>16)&255};};
  const writeStep=(track,step,s)=>mod._refm_wasm_set_acid_step(track,step,s.note,s.flags,s.prob||100);
  const noteLabel=n=>`${NOTE_NAMES[n%12]}${Math.floor(n/12)-1}`;
  const paintAcidStep=(track,step)=>{
    const b=document.querySelector(`#seq${track} .step[data-i="${step}"]`); if(!b)return;
    const s=readStep(track,step); b.className='step'+(s.flags&1?' on':'')+(s.flags&2?' accent':'')+(s.flags&4?' slide':'')+(s.flags&8?' tie':'');
    b.innerHTML=`<span class="refm-step-num">${step+1}</span><strong>${s.flags&1?noteLabel(s.note):'—'}</strong><small>${s.flags&2?'ACC ':''}${s.flags&4?'SLD ':''}${s.flags&8?'TIE':''}</small>`;
  };

  /* ACID modules: stock panel + x0x-style step/note programming drawer. */
  document.querySelectorAll('.acid').forEach((acid,track)=>{
    acid.dataset.refmMachine=`acid-${track}`;
    acid.querySelector('.acidTitle').textContent=track?'ACID B':'ACID A';
    const stock=document.createElement('div'); stock.className='refm-stock-strip';
    stock.innerHTML='<b>TB STYLE</b><span>SAW / SQUARE</span><span>4-STAGE DIODE VCF</span><span>ACCENT</span><span>LEGATO SLIDE</span><button class="refm-ext-toggle">EXT / MOD</button>';
    acid.querySelector('.acidHead')?.after(stock);
    const lfo=acid.querySelector('.lfoRow'); stock.querySelector('.refm-ext-toggle').onclick=()=>lfo?.classList.toggle('refm-mod-open');

    const editor=document.createElement('div'); editor.className='refm-note-editor'; editor.dataset.track=track;
    editor.innerHTML=`
      <div class="refm-editor-head"><div><b>STEP PROGRAMMER</b><span> select a step, then set pitch + flags</span></div><div class="refm-editor-readout">STEP <strong>01</strong> · <em>C3</em></div></div>
      <div class="refm-note-body">
        <div class="refm-oct"><button data-oct="-12">OCT −</button><span>OCTAVE</span><button data-oct="12">OCT +</button></div>
        <div class="refm-piano"></div>
        <div class="refm-flags"><button data-flag="1">GATE</button><button data-flag="2">ACCENT</button><button data-flag="4">SLIDE</button><button data-flag="8">TIE</button><button data-rest>REST</button></div>
      </div>`;
    acid.querySelector('.seqWrap')?.after(editor);
    let selected=0;
    const refreshEditor=()=>{
      const s=readStep(track,selected); editor.querySelector('.refm-editor-readout strong').textContent=String(selected+1).padStart(2,'0');
      editor.querySelector('.refm-editor-readout em').textContent=noteLabel(s.note||48);
      editor.querySelectorAll('[data-flag]').forEach(b=>b.classList.toggle('on',!!(s.flags&+b.dataset.flag)));
      editor.querySelectorAll('[data-note]').forEach(b=>b.classList.toggle('on',+b.dataset.note===(s.note||48)));
      document.querySelectorAll(`#seq${track} .step`).forEach((b,i)=>b.classList.toggle('selected',i===selected));
    };
    const piano=editor.querySelector('.refm-piano');
    for(let n=36;n<=72;n++){
      const b=document.createElement('button'); const black=[1,3,6,8,10].includes(n%12); b.className='refm-piano-key'+(black?' black':''); b.dataset.note=n; b.textContent=noteLabel(n);
      b.onclick=()=>{const s=readStep(track,selected);s.note=n;s.flags|=1;writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();noteOn(track,n);setTimeout(()=>noteOff(track,n),90);};piano.appendChild(b);
    }
    editor.querySelectorAll('[data-oct]').forEach(b=>b.onclick=()=>{const s=readStep(track,selected);s.note=clamp((s.note||48)+(+b.dataset.oct),24,96);s.flags|=1;writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();});
    editor.querySelectorAll('[data-flag]').forEach(b=>b.onclick=()=>{const s=readStep(track,selected),f=+b.dataset.flag;s.flags^=f;if(f!==8&&s.flags&(2|4))s.flags|=1;if(!(s.flags&1))s.flags&=~(2|4|8);writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();});
    editor.querySelector('[data-rest]').onclick=()=>{const s=readStep(track,selected);s.flags=0;writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();};
    document.querySelectorAll(`#seq${track} .step`).forEach((b,i)=>{b.onclick=e=>{e.preventDefault();selected=i;refreshEditor();};paintAcidStep(track,i);});
    refreshEditor();
  });

  /* Replace the single tabbed drum grid with two simultaneously visible rack machines. */
  const oldDrums=document.querySelector('.drums');
  if(oldDrums){
    const host=document.createElement('div');host.className='refm-drum-rack';
    [0,1].forEach(track=>{
      const sec=document.createElement('section');sec.className=`refm-drum-machine refm-drum-${track?'909':'808'}`;sec.dataset.refmDrumTrack=track;
      sec.innerHTML=`<div class="refm-drum-head"><div><strong>DRUM ${track?'909':'808'}</strong><small>RHYTHM MACHINE</small></div><div class="refm-drum-source-status">HYBRID</div></div><div class="refm-drum-grid"></div>`;
      const grid=sec.querySelector('.refm-drum-grid');
      DRUM_NAMES.forEach((name,lane)=>{
        const label=document.createElement('div');label.className='refm-drum-label';label.innerHTML=`<b>${name}</b><span>${['KICK','SNARE','CLAP','RIM','CLOSED','OPEN','LOW TOM','MID TOM','HIGH TOM','CRASH','RIDE'][lane]}</span>`;grid.appendChild(label);
        for(let step=0;step<16;step++){
          const b=document.createElement('button');b.className='refm-drum-step';b.dataset.lane=lane;b.dataset.step=step;b.innerHTML=`<i>${step+1}</i>`;
          b.onclick=()=>{const x=mod._refm_wasm_get_drum_step(track,step)>>>0;let hits=x&65535,acc=x>>>16,bit=1<<lane;if(!(hits&bit)){hits|=bit;acc&=~bit;}else if(!(acc&bit)){acc|=bit;}else{hits&=~bit;acc&=~bit;}mod._refm_wasm_set_drum_step(track,step,hits,acc);paintDrum(track,sec);};grid.appendChild(b);
        }
      });
      host.appendChild(sec);paintDrum(track,sec);
    });
    oldDrums.replaceWith(host);
  }

  function paintDrum(track,sec){
    sec.querySelectorAll('.refm-drum-step').forEach(b=>{const lane=+b.dataset.lane,step=+b.dataset.step,x=mod._refm_wasm_get_drum_step(track,step)>>>0,h=x&65535,a=x>>>16,bit=1<<lane;b.classList.toggle('hit',!!(h&bit));b.classList.toggle('accent',!!(a&bit));});
  }

  /* Actual firmware mixer/FX state, presented as a bottom hardware rack. */
  const mixer=document.createElement('section');mixer.className='refm-mixer-rack';
  mixer.innerHTML='<div class="refm-mixer-title">MIXER</div><div class="refm-channel-host"></div><div class="refm-fx-host"><h3>MASTER FX</h3></div><div class="refm-master-strip"><b>MASTER</b><div class="refm-master-meter"><i></i><i></i></div></div>';
  const chHost=mixer.querySelector('.refm-channel-host');
  TRACK_NAMES.forEach((name,track)=>{
    const ch=document.createElement('div');ch.className='refm-channel';ch.innerHTML=`<b>${name}</b><input class="refm-fader" type="range" min="0" max="32767" value="32767"><label>PAN<input data-pan type="range" min="-32767" max="32767" value="0"></label><div><button data-mute>M</button><button data-solo>S</button></div><label>SEND<input data-send type="range" min="0" max="127" value="0"></label>`;
    ch.querySelector('.refm-fader').oninput=e=>mod._refm_wasm_set_mix_track(track,0,+e.target.value);
    ch.querySelector('[data-pan]').oninput=e=>mod._refm_wasm_set_mix_track(track,1,+e.target.value);
    ch.querySelector('[data-send]').oninput=e=>mod._refm_wasm_set_mix_track(track,4,+e.target.value);
    ch.querySelector('[data-mute]').onclick=e=>{const on=!e.target.classList.toggle('on');e.target.classList.toggle('on',!on);mod._refm_wasm_set_mix_track(track,2,e.target.classList.contains('on')?1:0);};
    ch.querySelector('[data-solo]').onclick=e=>{e.target.classList.toggle('on');mod._refm_wasm_set_mix_track(track,3,e.target.classList.contains('on')?1:0);};
    chHost.appendChild(ch);
  });
  const fx=mixer.querySelector('.refm-fx-host');
  const fxDefs=[['DRIVE',0,0,20000,3000],['COMP',1,2048,32767,24576],['FILTER',2,256,32767,30000],['DELAY FB',3,0,30000,14000],['DELAY MIX',4,0,32767,7000],['DELAY TIME',5,64,2048,1102]];
  fxDefs.forEach(([name,param,min,max,val])=>{const c=document.createElement('label');c.className='refm-fx-control';c.innerHTML=`<span>${name}</span><input type="range" min="${min}" max="${max}" value="${val}"><i></i>`;c.querySelector('input').oninput=e=>mod._refm_wasm_set_fx(param,+e.target.value);fx.appendChild(c);});
  document.querySelector('.main')?.appendChild(mixer);

  let last=-1;setInterval(()=>{const step=mod._refm_wasm_step()&15;if(step===last)return;last=step;document.querySelectorAll('.step,.refm-drum-step').forEach(b=>b.classList.remove('playing'));document.querySelectorAll(`.step[data-i="${step}"],.refm-drum-step[data-step="${step}"]`).forEach(b=>b.classList.add('playing'));},50);
}
