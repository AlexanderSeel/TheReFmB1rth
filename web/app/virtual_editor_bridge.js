// SPDX-License-Identifier: GPL-3.0-only
const drumNotes=[36,38,39,37,42,46,41,45,48,49,51];
let machine=0,pattern=0,step=0,voice=0;
const send=b=>window.refmVirtual?.routeActive&&window.refmVirtual.sendMidi(b);
const cc=(n,v)=>send([0xB0+machine,n,Math.max(0,Math.min(127,Number(v)|0))]);
const later=fn=>queueMicrotask(fn);

document.addEventListener('click',e=>{
  const t=e.target.closest('button');if(!t)return;
  if(t.matches('.machine[data-machine]')){machine=Number(t.dataset.machine);return;}
  if(t.closest('#patterns')){const a=[...document.querySelectorAll('#patterns button')],i=a.indexOf(t);if(i>=0){pattern=i;send([0xC0+machine,i&7]);}return;}
  if(t.closest('#seqSteps')){const a=[...document.querySelectorAll('#seqSteps button')],i=a.indexOf(t);if(i>=0){step=i;cc(102,i);}return;}
  if(t.matches('[data-wave]')){cc(77,Number(t.dataset.wave)?127:0);return;}
  if(t.matches('[data-toggle-cc]')){later(()=>cc(Number(t.dataset.toggleCc),t.classList.contains('active')?127:0));return;}
  if(t.matches('[data-step-type]')){cc(102,step);cc(104,Number(t.dataset.stepType));return;}
  if(t.id==='stepAccent'||t.id==='stepSlide'){later(()=>{cc(102,step);const accent=document.querySelector('#stepAccent')?.classList.contains('active'),slide=document.querySelector('#stepSlide')?.classList.contains('active');cc(105,(accent?1:0)|(slide?2:0));});return;}
  if(t.closest('#voiceSelect')){const a=[...document.querySelectorAll('#voiceSelect button')],i=a.indexOf(t);if(i>=0){voice=i;cc(104,voice);}return;}
  if(t.matches('[data-drum-state]')){cc(102,step);cc(104,voice);cc(105,Number(t.dataset.drumState));return;}
  if(t.matches('[data-transport]')){const c=t.dataset.transport;send([c==='start'?0xFA:c==='continue'?0xFB:0xFC]);return;}
  if(t.closest('#keyboard')){const a=[...document.querySelectorAll('#keyboard button')],i=a.indexOf(t);if(i>=0&&machine<2){const note=36+i;send([0x90+machine,note,100]);setTimeout(()=>send([0x80+machine,note,0]),180);}return;}
  if(t.closest('#drumPads')){const a=[...document.querySelectorAll('#drumPads button')],i=a.indexOf(t);if(i>=0&&machine>=2){const note=drumNotes[i];send([0x90+machine,note,110]);setTimeout(()=>send([0x80+machine,note,0]),120);}}
});

document.addEventListener('input',e=>{
  const t=e.target;if(!(t instanceof HTMLInputElement))return;
  if(t.matches('input[data-cc]'))cc(Number(t.dataset.cc),t.value);
  else if(t.id==='stepProbability'){cc(102,step);cc(106,t.value);}
});
document.addEventListener('change',e=>{const t=e.target;if(t instanceof HTMLInputElement&&t.id==='stepNote'){cc(102,step);cc(103,t.value);}});

window.addEventListener('refm-virtual-route',e=>{if(!e.detail?.active)return;machine=Number(document.querySelector('.machine.active')?.dataset.machine||0);const p=[...document.querySelectorAll('#patterns button')].findIndex(x=>x.classList.contains('active'));if(p>=0)pattern=p;});
