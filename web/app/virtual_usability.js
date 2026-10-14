// SPDX-License-Identifier: GPL-3.0-only
// Virtual-only usability layer. Keeps physical FM-1 interaction untouched.

const NOTE_X0=6,NOTE_W=19,STEP_X0=5,STEP_W=14.4;
let display2x=false;

const state=()=>window.refmRB240State||null;
const send=bytes=>window.refmVirtual?.sendMidi?.(bytes);
const cc=(track,n,v)=>send([0xB0+(track&3),n,Math.max(0,Math.min(127,Number(v)|0))]);

function wheelKnob(label,dir,count=1){
  const knob=document.querySelector(`.vf-knob[aria-label="${label}"]`);if(!knob)return;
  for(let i=0;i<count;i++)knob.dispatchEvent(new WheelEvent('wheel',{bubbles:true,cancelable:true,deltaY:dir>0?-100:100}));
}
function moveWrapped(current,target,count,label){
  current=((current%count)+count)%count;target=((target%count)+count)%count;
  const up=(target-current+count)%count,down=(current-target+count)%count;
  if(up<=down)wheelKnob(label,1,up);else wheelKnob(label,-1,down);
}
function selectStep(target){const s=state();if(!s)return;moveWrapped(s.selectedStep,target,16,'PRESETS');cc(s.machine,102,target);}
function selectLane(target){const s=state();if(!s||s.machine<2)return;moveWrapped(s.lane,target,11,'KNOB1');cc(s.machine,104,target);}
function acidFlags(st){return(st.accent?1:0)|(st.slide?2:0);}
function currentAcidStep(s){return s?.acidSteps?.[s.selectedStep]||{note:48,type:1,accent:false,slide:false,prob:100};}

function editAcid(x,y,s){
  const track=s.machine;if(track>1)return false;
  if(y>=181&&y<=205){const i=Math.max(0,Math.min(15,Math.floor((x-STEP_X0)/STEP_W)));selectStep(i);return true;}
  const st=currentAcidStep(s);
  if(y>=99&&y<=119){
    if(x>=7&&x<51){cc(track,102,s.selectedStep);cc(track,104,0);return true;}
    if(x>=54&&x<98){cc(track,102,s.selectedStep);cc(track,104,1);return true;}
    if(x>=101&&x<145){cc(track,102,s.selectedStep);cc(track,104,2);return true;}
    if(x>=151&&x<190){cc(track,102,s.selectedStep);cc(track,105,acidFlags({...st,accent:!st.accent}));return true;}
    if(x>=193&&x<=234){cc(track,102,s.selectedStep);cc(track,105,acidFlags({...st,slide:!st.slide}));return true;}
  }
  if(y>=121&&y<=164&&x>=NOTE_X0&&x<NOTE_X0+NOTE_W*12){
    const pc=Math.max(0,Math.min(11,Math.floor((x-NOTE_X0)/NOTE_W)));
    let note=Math.floor(st.note/12)*12+pc;if(note<24)note+=12;if(note>96)note-=12;
    cc(track,102,s.selectedStep);cc(track,103,note);cc(track,104,1);return true;
  }
  return false;
}
function editDrum(x,y,s){
  if(s.machine<2)return false;
  if(y>=66&&y<=98){const lane=Math.max(0,Math.min(10,Math.floor((x-6)/20.7)));selectLane(lane);return true;}
  if(y>=103&&y<=158){
    const col=Math.floor((x-7)/28),row=Math.floor((y-104)/27);if(col<0||col>7||row<0||row>1)return false;
    const step=row*8+col;selectStep(step);const raw=s.drumSteps?.[step]||0,next=((raw&3)+1)%3;
    cc(s.machine,102,step);cc(s.machine,104,s.lane);cc(s.machine,105,next);return true;
  }
  return false;
}
function editAt(x,y){const s=state();if(!s?.ready)return false;if(s.page===2)return editAcid(x,y,s);if(s.page===3)return editDrum(x,y,s);return false;}
function logicalPoint(canvas,e){const r=canvas.getBoundingClientRect();return{x:(e.clientX-r.left)*240/r.width,y:(e.clientY-r.top)*240/r.height};}
function installCanvasEditing(canvas){canvas.addEventListener('pointerdown',e=>{const p=logicalPoint(canvas,e);if(editAt(p.x,p.y)){e.preventDefault();e.stopPropagation();}});}

function installQuickHelp(card){
  if(document.querySelector('#virtualDirectGuide'))return;
  const guide=document.createElement('article');guide.id='virtualDirectGuide';guide.className='card virtualDirectGuide';
  guide.innerHTML=`<div class="eyebrow">SIMPLE SEQUENCING</div><h3>Program patterns directly on the LCD</h3><div class="virtualDirectGuideGrid"><div><strong>ACID A / B</strong><ol><li>Open <b>SEQ</b>.</li><li>Click one of the 16 steps.</li><li>Click a note, or REST / NOTE / TIE.</li><li>Toggle ACC or SLD directly.</li></ol></div><div><strong>808 / 909</strong><ol><li>Open <b>ARP / DRUM</b>.</li><li>Click a drum lane.</li><li>Click steps to cycle OFF → HIT → ACCENT.</li><li>The grid always shows the real selected-lane pattern.</li></ol></div><div><strong>Display</strong><p><b>DISPLAY 2×</b> switches the LCD inside the FM-1 panel itself from a 240 × 240 backing surface to a true 480 × 480 backing surface. The UI is re-rendered at the higher pixel density; it is not a stretched screenshot.</p></div></div><p class="virtualDirectGuideNote">Modifier holds/chords remain available for performance shortcuts, but they are no longer required for normal step programming in the Virtual FM-1.</p>`;
  card.insertAdjacentElement('afterend',guide);
}

function applyDisplayDensity(canvas,button,badge){
  const px=display2x?480:240;
  if(canvas.width!==px)canvas.width=px;
  if(canvas.height!==px)canvas.height=px;
  canvas.dataset.renderPixels=String(px);
  canvas.setAttribute('aria-label',`FM-1 ${px} by ${px} high-density in-panel display`);
  button.classList.toggle('active',display2x);
  button.textContent=display2x?'DISPLAY 1×':'DISPLAY 2×';
  badge.textContent=display2x?'LCD 480 × 480 · HIGH DENSITY':'LCD 240 × 240';
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
    #virtualScreen{cursor:crosshair}.virtualZoomButton.active{border-color:#ffc75a;color:#ffe5aa}.virtualDisplayDensity{color:#8fd8e7;font-weight:700}.virtualDirectHint{color:#ffcf72!important}.virtualDirectGuide{margin-top:12px;border-color:#624e29}.virtualDirectGuide h3{margin:.25rem 0 1rem}.virtualDirectGuideGrid{display:grid;grid-template-columns:1fr 1fr .8fr;gap:12px}.virtualDirectGuideGrid>div{padding:12px;border:1px solid #303b45;border-radius:9px;background:#0b1117}.virtualDirectGuideGrid strong{color:#ffcf72}.virtualDirectGuideGrid ol{margin:.5rem 0 0 1.1rem;padding:0}.virtualDirectGuideGrid li{margin:.32rem 0}.virtualDirectGuideGrid p{margin:.5rem 0 0}.virtualDirectGuideNote{margin:12px 0 0;color:#9fb0bc}@media(max-width:850px){.virtualDirectGuideGrid{grid-template-columns:1fr}}
  `;document.head.appendChild(style);
  const hint=document.createElement('span');hint.className='virtualDirectHint';hint.textContent='SEQ/DRUM LCD is directly clickable';bar.appendChild(hint);
  applyDisplayDensity(primary,button,badge);
}

queueMicrotask(installDisplayDensity);
