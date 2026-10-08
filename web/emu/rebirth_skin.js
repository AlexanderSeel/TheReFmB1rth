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
  const writeStep=(track,step,s)=>mod._refm_wasm_set_acid_step(track,step,s.note,s.flags,clamp(s.prob??100,0,100));
  const noteLabel=n=>`${NOTE_NAMES[n%12]}${Math.floor(n/12)-1}`;
  const editorRefresh=[null,null];

  const paintAcidStep=(track,step)=>{
    const b=document.querySelector(`#seq${track} .step[data-i="${step}"]`); if(!b)return;
    const s=readStep(track,step);
    b.className='step'+(s.flags&1?' on':'')+(s.flags&2?' accent':'')+(s.flags&4?' slide':'')+(s.flags&8?' tie':'');
    b.innerHTML=`<span class="refm-step-num">${step+1}</span><strong>${s.flags&1?noteLabel(s.note):'—'}</strong><small>${s.flags&2?'ACC ':''}${s.flags&4?'SLD ':''}${s.flags&8?'TIE ':''}${s.prob<100?`${s.prob}%`:''}</small>`;
  };

  const showToast=(text,bad=false)=>{
    let t=document.querySelector('.refm-toast');
    if(!t){t=document.createElement('div');t.className='refm-toast';document.body.appendChild(t);}
    t.textContent=text;t.classList.toggle('bad',bad);t.classList.add('show');
    clearTimeout(t._timer);t._timer=setTimeout(()=>t.classList.remove('show'),2200);
  };

  /* Real rotary control layer over native ranges. */
  const knobify=(input)=>{
    if(!input || input.dataset.refmKnob==='1')return;
    input.dataset.refmKnob='1';
    const min=Number(input.min||0),max=Number(input.max||100),range=Math.max(1,max-min);
    const step=Math.max(Number(input.step||0)||range/127,range/1000);
    const reset=Number(input.defaultValue||input.value||((min+max)/2));
    const face=document.createElement('div');
    face.className='refm-knob';face.tabIndex=0;face.setAttribute('role','slider');
    face.title='Drag up/down · wheel · Shift=fine · double-click=reset';
    face.innerHTML='<span class="refm-knob-pointer"></span><small class="refm-knob-value"></small>';
    input.insertAdjacentElement('afterend',face);input.classList.add('refm-knob-input');
    const formattedValue=()=>{
      const value=Number(input.value),norm=clamp((value-min)/range,0,1);
      if(input.dataset.cc==='74'&&input.dataset.ch!==undefined)return `${mod._refm_wasm_acid_cutoff_hz(+input.dataset.ch)} Hz`;
      const fx=input.closest('.refm-fx-control')?.dataset.fxName;
      if(fx==='DELAY TIME')return `${Math.round(value/44.1)} ms`;
      if(fx==='COMP')return `${Math.round(norm*100)}%`;
      return `${Math.round(norm*100)}%`;
    };
    const sync=()=>{
      const value=Number(input.value),norm=clamp((value-min)/range,0,1),angle=-135+norm*270;
      face.style.setProperty('--angle',`${angle}deg`);face.setAttribute('aria-valuemin',String(min));face.setAttribute('aria-valuemax',String(max));face.setAttribute('aria-valuenow',String(value));face.querySelector('small').textContent=formattedValue();
    };
    const setValue=(v)=>{const snapped=Math.round((clamp(v,min,max)-min)/step)*step+min;input.value=String(clamp(snapped,min,max));input.dispatchEvent(new Event('input',{bubbles:true}));sync();};
    let startY=0,startValue=0,dragging=false;
    face.addEventListener('pointerdown',e=>{dragging=true;startY=e.clientY;startValue=Number(input.value);face.setPointerCapture(e.pointerId);face.classList.add('dragging');e.preventDefault();});
    face.addEventListener('pointermove',e=>{if(!dragging)return;const scale=e.shiftKey?.0018:.007;setValue(startValue+(startY-e.clientY)*range*scale);});
    const end=e=>{if(!dragging)return;dragging=false;try{face.releasePointerCapture(e.pointerId);}catch{}face.classList.remove('dragging');};
    face.addEventListener('pointerup',end);face.addEventListener('pointercancel',end);
    face.addEventListener('wheel',e=>{e.preventDefault();const mult=e.shiftKey?1:5;setValue(Number(input.value)+(e.deltaY<0?step*mult:-step*mult));},{passive:false});
    face.addEventListener('dblclick',()=>setValue(reset));
    face.addEventListener('keydown',e=>{let d=0;if(e.key==='ArrowUp'||e.key==='ArrowRight')d=step*(e.shiftKey?1:5);if(e.key==='ArrowDown'||e.key==='ArrowLeft')d=-step*(e.shiftKey?1:5);if(e.key==='Home'){e.preventDefault();return setValue(min);}if(e.key==='End'){e.preventDefault();return setValue(max);}if(d){e.preventDefault();setValue(Number(input.value)+d);}});
    input.addEventListener('input',sync);sync();
  };

  /* Pattern bank + JSON sequence exchange live in the visible top rack. */
  const seqTools=document.createElement('div');seqTools.className='refm-sequence-tools';
  seqTools.innerHTML='<div class="refm-pattern-tools"><b>PATTERN</b></div><button data-save-seq>SAVE JSON</button><button data-load-seq>LOAD JSON</button><input data-seq-file type="file" accept="application/json,.json" hidden>';
  document.querySelector('.master')?.appendChild(seqTools);
  const patternTools=seqTools.querySelector('.refm-pattern-tools');
  for(let i=0;i<8;i++){const b=document.createElement('button');b.dataset.pattern=i;b.textContent=String.fromCharCode(65+i);patternTools.appendChild(b);}

  function refreshPatternUI(){
    const p=mod._refm_wasm_pattern()&7;
    seqTools.querySelectorAll('[data-pattern]').forEach(b=>b.classList.toggle('on',+b.dataset.pattern===p));
    document.querySelectorAll('#patterns .btn').forEach((b,i)=>b.classList.toggle('on',i===p));
    const label=document.querySelector('#pattern');if(label)label.textContent=String.fromCharCode(65+p);
    for(let t=0;t<2;t++){for(let s=0;s<16;s++)paintAcidStep(t,s);editorRefresh[t]?.();}
    document.querySelectorAll('.refm-drum-machine').forEach(sec=>paintDrum(+sec.dataset.refmDrumTrack,sec));
  }
  seqTools.querySelectorAll('[data-pattern]').forEach(b=>b.onclick=()=>{mod._refm_wasm_select_pattern(+b.dataset.pattern);refreshPatternUI();window.dispatchEvent(new CustomEvent('refm:patternchange',{detail:{pattern:+b.dataset.pattern}}));});
  window.addEventListener('refm:patternchange',()=>queueMicrotask(refreshPatternUI));

  const captureSequence=()=>({
    format:'TheReFmB1rth.sequence',version:1,sourcePattern:mod._refm_wasm_pattern()&7,bpm:mod._refm_wasm_bpm(),
    acid:[0,1].map(t=>Array.from({length:16},(_,s)=>readStep(t,s))),
    drums:[0,1].map(t=>Array.from({length:16},(_,s)=>{const x=mod._refm_wasm_get_drum_step(t,s)>>>0;return{hits:x&0x7ff,accents:(x>>>16)&0x7ff};}))
  });
  seqTools.querySelector('[data-save-seq]').onclick=()=>{
    const data=captureSequence(),letter=String.fromCharCode(65+data.sourcePattern),blob=new Blob([JSON.stringify(data,null,2)+'\n'],{type:'application/json'}),url=URL.createObjectURL(blob),a=document.createElement('a');
    a.href=url;a.download=`TheReFmB1rth-pattern-${letter}.json`;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);showToast(`PATTERN ${letter} SAVED`);
  };
  const fileInput=seqTools.querySelector('[data-seq-file]');seqTools.querySelector('[data-load-seq]').onclick=()=>fileInput.click();
  fileInput.onchange=async()=>{
    const file=fileInput.files?.[0];if(!file)return;
    try{
      const data=JSON.parse(await file.text());
      if(data?.format!=='TheReFmB1rth.sequence'||data.version!==1)throw new Error('unsupported sequence format');
      if(!Array.isArray(data.acid)||data.acid.length!==2||!Array.isArray(data.drums)||data.drums.length!==2)throw new Error('invalid sequence lanes');
      for(let t=0;t<2;t++){if(!Array.isArray(data.acid[t])||data.acid[t].length!==16)throw new Error('acid lane must contain 16 steps');if(!Array.isArray(data.drums[t])||data.drums[t].length!==16)throw new Error('drum lane must contain 16 steps');}
      for(let t=0;t<2;t++)for(let s=0;s<16;s++){
        const a=data.acid[t][s]||{};mod._refm_wasm_set_acid_step(t,s,clamp(Number(a.note)||48,0,127),(Number(a.flags)||0)&15,clamp(Number(a.prob)??100,0,100));
        const d=data.drums[t][s]||{};mod._refm_wasm_set_drum_step(t,s,(Number(d.hits)||0)&0x7ff,(Number(d.accents)||0)&0x7ff);
      }
      if(Number.isFinite(Number(data.bpm))){const bpm=clamp(Math.round(Number(data.bpm)),60,200);mod._refm_wasm_set_bpm(bpm);const slider=document.querySelector('#bpm');if(slider)slider.value=String(bpm);const readout=document.querySelector('#bpmRead');if(readout)readout.textContent=String(bpm);}
      refreshPatternUI();
      const target=String.fromCharCode(65+(mod._refm_wasm_pattern()&7));showToast(`SEQUENCE LOADED INTO PATTERN ${target}`);
    }catch(err){console.error(err);showToast(`LOAD FAILED: ${err.message}`,true);}finally{fileInput.value='';}
  };

  /* ACID modules: stock panel + x0x-style step/note programming drawer. */
  document.querySelectorAll('.acid').forEach((acid,track)=>{
    acid.dataset.refmMachine=`acid-${track}`;acid.querySelector('.acidTitle').textContent=track?'ACID B':'ACID A';
    const stock=document.createElement('div');stock.className='refm-stock-strip';
    stock.innerHTML='<b>TB STYLE</b><span>SAW / SQUARE</span><span>4-STAGE DIODE VCF</span><span>ACCENT</span><span>LEGATO SLIDE</span><button class="refm-ext-toggle">EXT / MOD</button>';
    acid.querySelector('.acidHead')?.after(stock);const lfo=acid.querySelector('.lfoRow');stock.querySelector('.refm-ext-toggle').onclick=()=>lfo?.classList.toggle('refm-mod-open');

    const editor=document.createElement('div');editor.className='refm-note-editor';editor.dataset.track=track;
    editor.innerHTML=`<div class="refm-editor-head"><div><b>STEP PROGRAMMER</b><span> select step → pitch → accent/slide/tie</span></div><div class="refm-editor-readout">STEP <strong>01</strong> · <em>C3</em></div></div>
      <div class="refm-note-body"><div class="refm-oct"><button data-oct="-12">OCT −</button><span>OCTAVE</span><button data-oct="12">OCT +</button><div class="refm-prob"><span>PROB <output>100%</output></span><input data-prob type="range" min="0" max="100" value="100"></div></div><div class="refm-piano"></div><div class="refm-flags"><button data-flag="1">GATE</button><button data-flag="2">ACCENT</button><button data-flag="4">SLIDE</button><button data-flag="8">TIE</button><button data-rest>REST</button><button data-copy>COPY</button><button data-paste>PASTE</button></div></div>`;
    acid.querySelector('.seqWrap')?.after(editor);
    let selected=0,clipboard=null;
    const refreshEditor=()=>{
      const s=readStep(track,selected);editor.querySelector('.refm-editor-readout strong').textContent=String(selected+1).padStart(2,'0');editor.querySelector('.refm-editor-readout em').textContent=noteLabel(s.note||48);
      editor.querySelectorAll('[data-flag]').forEach(b=>b.classList.toggle('on',!!(s.flags&+b.dataset.flag)));editor.querySelectorAll('[data-note]').forEach(b=>b.classList.toggle('on',+b.dataset.note===(s.note||48)));
      const prob=editor.querySelector('[data-prob]');prob.value=String(s.prob);editor.querySelector('.refm-prob output').textContent=`${s.prob}%`;
      document.querySelectorAll(`#seq${track} .step`).forEach((b,i)=>b.classList.toggle('selected',i===selected));
    };
    editorRefresh[track]=refreshEditor;
    const piano=editor.querySelector('.refm-piano');
    for(let n=36;n<=72;n++){const b=document.createElement('button'),black=[1,3,6,8,10].includes(n%12);b.className='refm-piano-key'+(black?' black':'');b.dataset.note=n;b.textContent=noteLabel(n);b.onclick=()=>{const s=readStep(track,selected);s.note=n;s.flags|=1;writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();noteOn(track,n);setTimeout(()=>noteOff(track,n),90);};piano.appendChild(b);}
    editor.querySelectorAll('[data-oct]').forEach(b=>b.onclick=()=>{const s=readStep(track,selected);s.note=clamp((s.note||48)+(+b.dataset.oct),24,96);s.flags|=1;writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();});
    editor.querySelectorAll('[data-flag]').forEach(b=>b.onclick=()=>{const s=readStep(track,selected),f=+b.dataset.flag;s.flags^=f;if(f!==8&&s.flags&(2|4))s.flags|=1;if(!(s.flags&1))s.flags&=~(2|4|8);writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();});
    editor.querySelector('[data-rest]').onclick=()=>{const s=readStep(track,selected);s.flags=0;writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();};
    editor.querySelector('[data-copy]').onclick=()=>{clipboard={...readStep(track,selected)};showToast(`ACID ${track?'B':'A'} STEP ${selected+1} COPIED`);};
    editor.querySelector('[data-paste]').onclick=()=>{if(!clipboard)return showToast('COPY A STEP FIRST',true);writeStep(track,selected,{...clipboard});paintAcidStep(track,selected);refreshEditor();};
    editor.querySelector('[data-prob]').oninput=e=>{const s=readStep(track,selected);s.prob=clamp(+e.target.value,0,100);writeStep(track,selected,s);paintAcidStep(track,selected);refreshEditor();};
    document.querySelectorAll(`#seq${track} .step`).forEach((b,i)=>{b.onclick=e=>{e.preventDefault();selected=i;refreshEditor();};paintAcidStep(track,i);});refreshEditor();
  });

  /* Replace the single tabbed drum grid with two simultaneously visible rack machines. */
  const oldDrums=document.querySelector('.drums');
  if(oldDrums){
    const host=document.createElement('div');host.className='refm-drum-rack';
    [0,1].forEach(track=>{
      const sec=document.createElement('section');sec.className=`refm-drum-machine refm-drum-${track?'909':'808'}`;sec.dataset.refmDrumTrack=track;
      sec.innerHTML=`<div class="refm-drum-head"><div><strong>DRUM ${track?'909':'808'}</strong><small>RHYTHM MACHINE · click cycles OFF/HIT/ACCENT · right-click clears</small></div><div class="refm-drum-source-status">HYBRID</div></div><div class="refm-drum-grid"></div>`;
      const grid=sec.querySelector('.refm-drum-grid');
      DRUM_NAMES.forEach((name,lane)=>{
        const label=document.createElement('div');label.className='refm-drum-label';label.innerHTML=`<b>${name}</b><span>${['KICK','SNARE','CLAP','RIM','CLOSED','OPEN','LOW TOM','MID TOM','HIGH TOM','CRASH','RIDE'][lane]}</span>`;grid.appendChild(label);
        for(let step=0;step<16;step++){
          const b=document.createElement('button');b.className='refm-drum-step';b.dataset.lane=lane;b.dataset.step=step;b.innerHTML=`<i>${step+1}</i>`;
          b.onclick=()=>{const x=mod._refm_wasm_get_drum_step(track,step)>>>0;let hits=x&65535,acc=x>>>16,bit=1<<lane;if(!(hits&bit)){hits|=bit;acc&=~bit;}else if(!(acc&bit)){acc|=bit;}else{hits&=~bit;acc&=~bit;}mod._refm_wasm_set_drum_step(track,step,hits,acc);paintDrum(track,sec);};
          b.oncontextmenu=e=>{e.preventDefault();const x=mod._refm_wasm_get_drum_step(track,step)>>>0,bit=1<<lane;mod._refm_wasm_set_drum_step(track,step,(x&65535)&~bit,(x>>>16)&~bit);paintDrum(track,sec);};grid.appendChild(b);
        }
      });host.appendChild(sec);paintDrum(track,sec);
    });oldDrums.replaceWith(host);
  }

  function paintDrum(track,sec){sec.querySelectorAll('.refm-drum-step').forEach(b=>{const lane=+b.dataset.lane,step=+b.dataset.step,x=mod._refm_wasm_get_drum_step(track,step)>>>0,h=x&65535,a=x>>>16,bit=1<<lane;b.classList.toggle('hit',!!(h&bit));b.classList.toggle('accent',!!(a&bit));});}

  /* Actual firmware mixer/FX state, presented as a bottom hardware rack. */
  const mixer=document.createElement('section');mixer.className='refm-mixer-rack';
  mixer.innerHTML='<div class="refm-mixer-title">MIXER</div><div class="refm-channel-host"></div><div class="refm-fx-host"><h3>MASTER FX</h3></div><div class="refm-master-strip"><b>MASTER</b><div class="refm-master-meter"><i></i><i></i></div><small class="refm-master-peak">0.000</small></div>';
  const chHost=mixer.querySelector('.refm-channel-host');
  TRACK_NAMES.forEach((name,track)=>{
    const level=mod._refm_wasm_get_mix_track(track,0),pan=mod._refm_wasm_get_mix_track(track,1),mute=mod._refm_wasm_get_mix_track(track,2),solo=mod._refm_wasm_get_mix_track(track,3),send=mod._refm_wasm_get_mix_track(track,4);
    const ch=document.createElement('div');ch.className='refm-channel';ch.innerHTML=`<b>${name}</b><input class="refm-fader" type="range" min="0" max="32767" value="${level}"><output data-level>${Math.round(level/32767*100)}</output><label>PAN<input data-pan type="range" min="-32767" max="32767" value="${pan}"></label><div><button data-mute class="${mute?'on':''}">M</button><button data-solo class="${solo?'on':''}">S</button></div><label>SEND<input data-send type="range" min="0" max="127" value="${send}"></label>`;
    ch.querySelector('.refm-fader').oninput=e=>{mod._refm_wasm_set_mix_track(track,0,+e.target.value);ch.querySelector('[data-level]').textContent=Math.round(+e.target.value/32767*100);};
    ch.querySelector('[data-pan]').oninput=e=>mod._refm_wasm_set_mix_track(track,1,+e.target.value);ch.querySelector('[data-send]').oninput=e=>mod._refm_wasm_set_mix_track(track,4,+e.target.value);
    ch.querySelector('[data-mute]').onclick=e=>{e.target.classList.toggle('on');mod._refm_wasm_set_mix_track(track,2,e.target.classList.contains('on')?1:0);};ch.querySelector('[data-solo]').onclick=e=>{e.target.classList.toggle('on');mod._refm_wasm_set_mix_track(track,3,e.target.classList.contains('on')?1:0);};chHost.appendChild(ch);
  });
  const fx=mixer.querySelector('.refm-fx-host');
  const fxDefs=[['DRIVE',0,0,20000],['COMP',1,2048,32767],['FILTER',2,256,32767],['DELAY FB',3,0,30000],['DELAY MIX',4,0,32767],['DELAY TIME',5,64,2048]];
  fxDefs.forEach(([name,param,min,max])=>{const val=mod._refm_wasm_get_fx(param),c=document.createElement('label');c.className='refm-fx-control';c.dataset.fxName=name;c.innerHTML=`<span>${name}</span><input type="range" min="${min}" max="${max}" value="${val}">`;c.querySelector('input').oninput=e=>mod._refm_wasm_set_fx(param,+e.target.value);fx.appendChild(c);});
  document.querySelector('.main')?.appendChild(mixer);

  document.querySelectorAll('.ctl input[type=range],.refm-fx-control input[type=range]').forEach(knobify);

  let lastStep=-1,lastPattern=mod._refm_wasm_pattern()&7;
  refreshPatternUI();
  setInterval(()=>{
    const step=mod._refm_wasm_step()&15,pattern=mod._refm_wasm_pattern()&7;
    if(pattern!==lastPattern){lastPattern=pattern;refreshPatternUI();}
    if(step!==lastStep){lastStep=step;document.querySelectorAll('.step,.refm-drum-step').forEach(b=>b.classList.remove('playing'));document.querySelectorAll(`.step[data-i="${step}"],.refm-drum-step[data-step="${step}"]`).forEach(b=>b.classList.add('playing'));}
    const peak=clamp(Number(document.querySelector('#peak')?.textContent)||0,0,1),height=Math.max(2,peak*100);mixer.querySelectorAll('.refm-master-meter i').forEach(i=>i.style.height=`${height}%`);mixer.querySelector('.refm-master-peak').textContent=peak.toFixed(3);
  },50);
}