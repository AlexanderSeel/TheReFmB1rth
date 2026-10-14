// SPDX-License-Identifier: GPL-3.0-only
// Virtual-only usability layer. Keeps physical FM-1 interaction untouched.

const NOTE_X0=6,NOTE_W=17.45,STEP_X0=5,STEP_W=14.4;
const GATE=1,ACCENT=2,SLIDE=4,TIE=8,DOWN=16,UP=32;
const TB_MIN=48,TB_NORMAL_LOW=60,TB_NORMAL_HIGH=72,TB_MAX=84;
let display2x=false;
const drumShadow=new Map();

const state=()=>window.refmRB240State||null;
const send=bytes=>window.refmVirtual?.sendMidi?.(bytes);
const cc=(track,n,v)=>send([0xB0+(track&3),n,Math.max(0,Math.min(127,Number(v)|0))]);
const clamp303=n=>Math.max(TB_MIN,Math.min(TB_MAX,Number(n)|0));
const drumShadowKey=(machine,step,lane)=>`${machine}:${step&15}:${lane}`;

function virtualStatus(text){
  const el=document.querySelector('#virtualStatus');
  if(el){el.textContent=text;el.classList.add('online');}
}
function wheelKnob(label,dir,count=1){
  const knob=document.querySelector(`.vf-knob[aria-label="${label}"]`);if(!knob)return false;
  for(let i=0;i<count;i++)knob.dispatchEvent(new WheelEvent('wheel',{bubbles:true,cancelable:true,deltaY:dir>0?-100:100}));
  return true;
}
function moveWrapped(current,target,count,label){
  current=((current%count)+count)%count;target=((target%count)+count)%count;
  const up=(target-current+count)%count,down=(current-target+count)%count;
  if(up<=down)wheelKnob(label,1,up);else wheelKnob(label,-1,down);
}
function selectStep(target){const s=state();if(!s)return;moveWrapped(s.selectedStep,target,16,'PRESETS');cc(s.machine,102,target);}
function selectLane(target){const s=state();if(!s||s.machine<2)return;moveWrapped(s.lane,target,11,'KNOB1');cc(s.machine,104,target);}
function currentAcidStep(s){return s?.acidSteps?.[s.selectedStep]||{note:60,type:1,accent:false,slide:false,prob:100,down:false,up:false};}
function octaveMode(st){if(st.down&&!st.up)return-1;if(st.up&&!st.down)return 1;if(st.note>=TB_MIN&&st.note<TB_NORMAL_LOW)return-1;if(st.note>TB_NORMAL_HIGH&&st.note<=TB_MAX)return 1;return 0;}
function base303Note(st){let n=(st.note>=TB_MIN&&st.note<=TB_MAX?st.note:TB_NORMAL_LOW)-octaveMode(st)*12;while(n<TB_NORMAL_LOW)n+=12;while(n>TB_NORMAL_HIGH)n-=12;return Math.max(TB_NORMAL_LOW,Math.min(TB_NORMAL_HIGH,n));}
function effective303Note(base,mode){return clamp303(base+(mode<0?-12:mode>0?12:0));}
function acidWasmFlags(st){const mode=st.octave??octaveMode(st);return(st.type===1?GATE:0)|(st.type===2?TIE:0)|(st.accent?ACCENT:0)|(st.slide?SLIDE:0)|(mode<0?DOWN:0)|(mode>0?UP:0);}
function commitAcid(track,step,st){
  const api=window.refmVirtual,note=clamp303(st.note),next={...st,note};
  if(api?.setAcidStep)return api.setAcidStep(track,step,note,acidWasmFlags(next),next.prob);
  cc(track,102,step);cc(track,103,note);cc(track,104,next.type);cc(track,105,(next.accent?1:0)|(next.slide?2:0));cc(track,106,next.prob);return true;
}
function setOctaveMode(track,step,st,mode){
  mode=Math.max(-1,Math.min(1,mode|0));
  const base=base303Note(st),note=effective303Note(base,mode);
  return commitAcid(track,step,{...st,note,octave:mode,down:mode<0,up:mode>0,type:st.type===0?1:st.type});
}
function shiftSelectedOctave(dir){
  const s=state();if(!s||s.machine>1||s.page!==2)return false;
  const st=currentAcidStep(s),current=octaveMode(st),wanted=dir<0?(current===-1?0:-1):(current===1?0:1);
  return setOctaveMode(s.machine,s.selectedStep,st,wanted);
}
function drumLaneState(machine,step,lane,s){
  const api=window.refmVirtual,d=machine-2,bit=1<<lane,key=drumShadowKey(machine,step,lane);
  if(api?.getDrumStep){
    const packed=api.getDrumStep(d,step),hits=packed&0xffff,accents=(packed>>>16)&0xffff;
    const current=(accents&bit)?2:(hits&bit)?1:0;
    drumShadow.set(key,current);
    return current;
  }
  if(drumShadow.has(key))return drumShadow.get(key);
  const current=(s?.drumSteps?.[step]||0)&3;
  drumShadow.set(key,current);
  return current;
}
function commitDrum(machine,step,lane,next,s=state()){
  const api=window.refmVirtual,d=machine-2,bit=1<<lane,key=drumShadowKey(machine,step,lane);
  next=Math.max(0,Math.min(2,next|0));
  let wrote=false;
  if(api?.getDrumStep&&api?.setDrumStep){
    const packed=api.getDrumStep(d,step);
    let hits=packed&0xffff,accents=(packed>>>16)&0xffff;
    if(next===0){hits&=~bit;accents&=~bit;}
    else{hits|=bit;if(next===2)accents|=bit;else accents&=~bit;}
    wrote=api.setDrumStep(d,step,hits,accents)!==false;
    if(wrote){
      const check=api.getDrumStep(d,step),checkHits=check&0xffff,checkAcc=(check>>>16)&0xffff;
      const actual=(checkAcc&bit)?2:(checkHits&bit)?1:0;
      if(actual!==next)wrote=false;
    }
  }
  if(!wrote){
    cc(machine,102,step);cc(machine,104,lane);cc(machine,105,next);wrote=true;
  }
  drumShadow.set(key,next);
  // Keep the on-screen state deterministic even when an older cached WASM lacks
  // the 16-step readback ABI. New WASM will replace this with authoritative data
  // on the next draw; the shadow still guarantees the next click can switch off.
  const live=window.refmRB240State;
  if(live?.drumSteps){
    const flam=(live.drumSteps[step]||0)&4;
    live.selectedStep=step;
    live.drumSteps[step]=flam|next;
    live.drumState=next;
  }
  api?.draw?.();
  const names=machine===3?['BD','SD','CP','RS','CH','OH','LT','MT','HT','CR','RD']:['BD','SD','CP','RS','CH','OH','LT','MT','HT','CY','CB'];
  virtualStatus(`DRUM ${machine===3?'909':'808'} · ${names[lane]||lane} · STEP ${step+1} · ${['OFF','HIT','ACC'][next]}`);
  return wrote;
}
function cycleDrum(machine,step,lane,s=state()){
  const current=drumLaneState(machine,step,lane,s),next=(current+1)%3;
  return commitDrum(machine,step,lane,next,s);
}
function clickKnobCard(x,y,x0,y0,w,h){
  if(y<y0||y>y0+h||x<x0||x>x0+4*w)return false;
  const slot=Math.max(0,Math.min(3,Math.floor((x-x0)/w))),local=(x-(x0+slot*w))/w;
  return wheelKnob(`KNOB${slot+1}`,local>=.5?1:-1,1);
}

function editAcid(x,y,s){
  const track=s.machine;if(track>1)return false;
  if(y>=181&&y<=205){const i=Math.max(0,Math.min(15,Math.floor((x-STEP_X0)/STEP_W)));selectStep(i);return true;}
  const st=currentAcidStep(s),step=s.selectedStep;
  if(y>=99&&y<=119){
    if(x>=7&&x<49){commitAcid(track,step,{...st,type:0});return true;}
    if(x>=52&&x<94){commitAcid(track,step,{...st,type:1});return true;}
    if(x>=97&&x<135){const m=octaveMode(st);setOctaveMode(track,step,st,m===-1?0:-1);return true;}
    if(x>=138&&x<176){const m=octaveMode(st);setOctaveMode(track,step,st,m===1?0:1);return true;}
    if(x>=179&&x<205){commitAcid(track,step,{...st,accent:!st.accent});return true;}
    if(x>=208&&x<=234){commitAcid(track,step,{...st,slide:!st.slide});return true;}
  }
  if(y>=121&&y<=164&&x>=NOTE_X0&&x<NOTE_X0+NOTE_W*13){
    const key=Math.max(0,Math.min(12,Math.floor((x-NOTE_X0)/NOTE_W))),base=TB_NORMAL_LOW+key,mode=octaveMode(st);
    commitAcid(track,step,{...st,note:effective303Note(base,mode),octave:mode,down:mode<0,up:mode>0,type:1});return true;
  }
  return false;
}
function editDrum(x,y,s){
  if(s.machine<2)return false;
  if(y>=66&&y<=98){const lane=Math.max(0,Math.min(10,Math.floor((x-6)/20.7)));selectLane(lane);return true;}
  if(y>=103&&y<=158){
    const col=Math.floor((x-7)/28),row=Math.floor((y-104)/27);if(col<0||col>7||row<0||row>1)return false;
    const step=row*8+col;selectStep(step);cycleDrum(s.machine,step,s.lane,s);return true;
  }
  // The STATE card itself is now an explicit control as well. This removes any
  // ambiguity about whether the selected step is OFF/HIT/ACC and makes touch use
  // considerably easier than having to hit the small grid cell repeatedly.
  if(y>=163&&y<=198&&x>=104&&x<164){cycleDrum(s.machine,s.selectedStep,s.lane,s);return true;}
  if(y>=163&&y<=198&&s.machine===3&&x>=164){wheelKnob('KNOB3',1,1);return true;}
  return false;
}
function editSettings(x,y,s){
  if(s.page===1){
    if(clickKnobCard(x,y,4,64,58,62))return true;
    if(y>=147&&y<=166&&x>=10&&x<=72){document.querySelector('.vf-button[aria-label="EDIT"]')?.dispatchEvent(new PointerEvent('pointerdown',{bubbles:true,pointerId:999}));document.querySelector('.vf-button[aria-label="EDIT"]')?.dispatchEvent(new PointerEvent('pointerup',{bubbles:true,pointerId:999}));return true;}
  }
  if(s.page===4){if(clickKnobCard(x,y,4,65,56,90))return true;}
  if(s.page===5){
    if(y>=65&&y<=84){const target=Math.max(0,Math.min(3,Math.floor((x-5)/58)));moveWrapped(s.fx,target,4,'ALGORITHM');return true;}
    if(clickKnobCard(x,y,8,92,55,86))return true;
  }
  return false;
}
function editAt(x,y){const s=state();if(!s?.ready)return false;if(s.page===2)return editAcid(x,y,s);if(s.page===3)return editDrum(x,y,s);if(s.page===1||s.page===4||s.page===5)return editSettings(x,y,s);return false;}
function logicalPoint(canvas,e){const r=canvas.getBoundingClientRect();return{x:(e.clientX-r.left)*240/r.width,y:(e.clientY-r.top)*240/r.height};}
function installCanvasEditing(canvas){canvas.addEventListener('pointerdown',e=>{const p=logicalPoint(canvas,e);if(editAt(p.x,p.y)){e.preventDefault();e.stopPropagation();}});}

function installQuickHelp(card){
  if(document.querySelector('#virtualDirectGuide'))return;
  const guide=document.createElement('article');guide.id='virtualDirectGuide';guide.className='card virtualDirectGuide';
  guide.innerHTML=`<div class="eyebrow">REBIRTH-STYLE SEQUENCING</div><h3>Program the groovebox directly</h3><div class="virtualDirectGuideGrid"><div><strong>ACID A / B</strong><ol><li>Open <b>SEQ</b> and select one of 16 steps.</li><li>Choose one pitch from the single C…C keyboard.</li><li>Use <b>DOWN</b> or <b>UP</b> to transpose that step by one octave. Normal is neither.</li><li>Set NOTE/PAUSE, ACCENT and SLIDE.</li></ol><p>The TB-303/ReBirth register is DOWN / NORMAL / UP; there is no generic C1…C9 piano range.</p></div><div><strong>808 / 909</strong><ol><li>Open <b>ARP / DRUM</b>.</li><li>Click a drum lane.</li><li>Click steps or the STATE card to cycle OFF → HIT → ACCENT → OFF.</li><li>K1 changes lane, K2 changes state; 909 K3/K4 control flam.</li></ol></div><div><strong>Settings</strong><p>ACID, MIXER and FX LCD cards are directly clickable: left half decreases, right half increases. The physical K1–K4 knobs perform the same edits.</p><p><b>DISPLAY 2×</b> uses the same integrated LCD at 480 × 480 backing resolution.</p></div></div><p class="virtualDirectGuideNote">Virtual editing now uses the same underlying groovebox state as the hardware control path.</p>`;
  card.insertAdjacentElement('afterend',guide);
}

function applyDisplayDensity(canvas,button,badge){
  const px=display2x?480:240,device=document.querySelector('#virtualDevice');
  if(canvas.width!==px)canvas.width=px;
  if(canvas.height!==px)canvas.height=px;
  canvas.dataset.renderPixels=String(px);
  canvas.style.imageRendering=display2x?'auto':'pixelated';
  canvas.setAttribute('aria-label',`FM-1 ${px} by ${px} high-density in-panel display`);
  device?.classList.toggle('display2x',display2x);
  button.classList.toggle('active',display2x);
  button.textContent=display2x?'DISPLAY 1×':'DISPLAY 2×';
  badge.textContent=display2x?'LCD 480 × 480 · LARGE PANEL':'LCD 240 × 240';
  window.refmVirtual?.draw?.();
}

function installDisplayDensity(){
  const primary=document.querySelector('#virtualScreen'),bar=document.querySelector('.virtualBar'),card=document.querySelector('.virtualCard');if(!primary||!bar||!card)return;
  const button=document.createElement('button');button.type='button';button.className='secondary virtualZoomButton';button.textContent='DISPLAY 2×';
  const badge=document.createElement('span');badge.className='virtualDisplayDensity';badge.textContent='LCD 240 × 240';
  bar.append(button,badge);
  installCanvasEditing(primary);installQuickHelp(card);
  button.addEventListener('click',()=>{display2x=!display2x;applyDisplayDensity(primary,button,badge);});
  const style=document.createElement('style');style.textContent=`
    #virtualScreen{cursor:crosshair}.virtualZoomButton.active{border-color:#ffc75a;color:#ffe5aa}.virtualDisplayDensity{color:#8fd8e7;font-weight:700}.virtualDirectHint{color:#ffcf72!important}
    .virtualDevice.display2x{aspect-ratio:1000/1040;background:linear-gradient(145deg,#565a5e 0%,#2b2e32 34%,#15171a 35%,#15171a 100%);box-shadow:0 28px 82px #000b,inset 0 1px 0 #8c9094}
    .virtualDevice.display2x::before{content:"HIGH-DENSITY LCD";position:absolute;z-index:1;left:4%;right:4%;top:1.2%;height:37%;border:1px solid #73777b;border-radius:26px;background:linear-gradient(160deg,#44484c,#26292d 54%,#3a3e42);box-shadow:inset 0 1px 0 #8d9296,inset 0 -12px 30px #0008;color:#c8cbce;font:700 12px ui-monospace,monospace;letter-spacing:.18em;padding:12px 16px;box-sizing:border-box}
    .virtualDevice.display2x>svg{inset:auto 0 0 0;height:61.54%;width:100%;z-index:2}
    .virtualDevice.display2x>#virtualScreen{left:30%;top:2.7%;width:40%;height:auto;aspect-ratio:1/1;z-index:4;border-radius:3px;box-shadow:0 0 0 7px #101113,0 0 0 10px #666b70,0 12px 34px #000d}
    .virtualDirectGuide{margin-top:12px;border-color:#624e29}.virtualDirectGuide h3{margin:.25rem 0 1rem}.virtualDirectGuideGrid{display:grid;grid-template-columns:1fr 1fr .8fr;gap:12px}.virtualDirectGuideGrid>div{padding:12px;border:1px solid #303b45;border-radius:9px;background:#0b1117}.virtualDirectGuideGrid strong{color:#ffcf72}.virtualDirectGuideGrid ol{margin:.5rem 0 0 1.1rem;padding:0}.virtualDirectGuideGrid li{margin:.32rem 0}.virtualDirectGuideGrid p{margin:.5rem 0 0}.virtualDirectGuideNote{margin:12px 0 0;color:#9fb0bc}@media(max-width:850px){.virtualDirectGuideGrid{grid-template-columns:1fr}.virtualDevice.display2x>#virtualScreen{left:25%;width:50%;top:2%}}
  `;document.head.appendChild(style);
  const hint=document.createElement('span');hint.className='virtualDirectHint';hint.textContent='LCD: SEQ, DRUM, ACID, MIXER and FX are directly editable';bar.appendChild(hint);
  applyDisplayDensity(primary,button,badge);
  queueMicrotask(()=>{if(window.refmVirtual)window.refmVirtual.shiftAcidOctave=shiftSelectedOctave;});
}

queueMicrotask(installDisplayDensity);