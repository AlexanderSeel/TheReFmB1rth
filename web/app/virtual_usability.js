// SPDX-License-Identifier: GPL-3.0-only
// Virtual-only usability layer. Keeps physical FM-1 interaction untouched.

const NOTE_X0=6,NOTE_W=19,STEP_X0=5,STEP_W=14.4;
let zoomCanvas=null,zoomWrap=null,zoomOn=false;

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
  const track=s.machine;if(track>1)return;
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

function installZoom(){
  const primary=document.querySelector('#virtualScreen'),bar=document.querySelector('.virtualBar');if(!primary||!bar)return;
  const button=document.createElement('button');button.type='button';button.className='secondary virtualZoomButton';button.textContent='DISPLAY 2×';
  zoomWrap=document.createElement('div');zoomWrap.className='virtualDisplayZoom';zoomWrap.hidden=true;
  const head=document.createElement('div');head.className='virtualDisplayZoomHead';head.innerHTML='<strong>FM-1 LCD · 2× ZOOM</strong><span>480 × 480</span>';
  zoomCanvas=document.createElement('canvas');zoomCanvas.width=480;zoomCanvas.height=480;zoomCanvas.setAttribute('aria-label','FM-1 display 2x zoom 480 by 480');
  zoomWrap.append(head,zoomCanvas);document.querySelector('.virtualCard')?.appendChild(zoomWrap);bar.appendChild(button);
  installCanvasEditing(primary);installCanvasEditing(zoomCanvas);
  button.addEventListener('click',()=>{zoomOn=!zoomOn;zoomWrap.hidden=!zoomOn;button.classList.toggle('active',zoomOn);button.textContent=zoomOn?'DISPLAY 1×':'DISPLAY 2×';syncZoom();});
  const style=document.createElement('style');style.textContent=`
    #virtualScreen{cursor:crosshair}.virtualDisplayZoom{margin:14px auto 2px;max-width:520px;padding:12px;border:1px solid #3b4853;border-radius:14px;background:#090d11;box-shadow:0 18px 44px #0008}.virtualDisplayZoom[hidden]{display:none}.virtualDisplayZoomHead{display:flex;justify-content:space-between;gap:12px;margin-bottom:8px;color:#dce8ef;font:700 11px ui-monospace,monospace}.virtualDisplayZoom canvas{display:block;width:min(480px,100%);height:auto;aspect-ratio:1;background:#17191b;image-rendering:pixelated;border:5px solid #30363b;box-shadow:0 0 0 2px #0c0f11;cursor:crosshair}.virtualZoomButton.active{border-color:#ffc75a;color:#ffe5aa}.virtualDirectHint{color:#ffcf72!important}
  `;document.head.appendChild(style);
  const hint=document.createElement('span');hint.className='virtualDirectHint';hint.textContent='SEQ/DRUM LCD is directly clickable';bar.appendChild(hint);
  setInterval(syncZoom,50);
}
function syncZoom(){if(!zoomOn||!zoomCanvas)return;const src=document.querySelector('#virtualScreen');if(!src)return;const g=zoomCanvas.getContext('2d');g.imageSmoothingEnabled=false;g.clearRect(0,0,480,480);g.drawImage(src,0,0,480,480);}

queueMicrotask(installZoom);
