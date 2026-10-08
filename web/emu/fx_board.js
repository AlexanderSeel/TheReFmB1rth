const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));

function knobify(input,format){
  const min=Number(input.min||0),max=Number(input.max||100),range=Math.max(1,max-min);
  const step=Math.max(Number(input.step||0)||range/127,range/1000);
  const reset=Number(input.value||((min+max)/2));
  const face=document.createElement('div');
  face.className='refm-knob';face.tabIndex=0;face.setAttribute('role','slider');
  face.title='Drag up/down · wheel · Shift=fine · double-click=reset';
  face.innerHTML='<span class="refm-knob-pointer"></span><small class="refm-knob-value"></small>';
  input.insertAdjacentElement('afterend',face);input.classList.add('refm-knob-input');
  const sync=()=>{
    const value=Number(input.value),norm=clamp((value-min)/range,0,1),angle=-135+norm*270;
    face.style.setProperty('--angle',`${angle}deg`);face.setAttribute('aria-valuemin',String(min));face.setAttribute('aria-valuemax',String(max));face.setAttribute('aria-valuenow',String(value));
    face.querySelector('small').textContent=format?format(value,norm):`${Math.round(norm*100)}%`;
  };
  const setValue=v=>{const snapped=Math.round((clamp(v,min,max)-min)/step)*step+min;input.value=String(clamp(snapped,min,max));input.dispatchEvent(new Event('input',{bubbles:true}));sync();};
  let startY=0,startValue=0,dragging=false;
  face.addEventListener('pointerdown',e=>{dragging=true;startY=e.clientY;startValue=Number(input.value);face.setPointerCapture(e.pointerId);face.classList.add('dragging');e.preventDefault();});
  face.addEventListener('pointermove',e=>{if(!dragging)return;setValue(startValue+(startY-e.clientY)*range*(e.shiftKey?.0018:.007));});
  const end=e=>{if(!dragging)return;dragging=false;try{face.releasePointerCapture(e.pointerId);}catch{}face.classList.remove('dragging');};
  face.addEventListener('pointerup',end);face.addEventListener('pointercancel',end);
  face.addEventListener('wheel',e=>{e.preventDefault();setValue(Number(input.value)+(e.deltaY<0?1:-1)*step*(e.shiftKey?1:5));},{passive:false});
  face.addEventListener('dblclick',()=>setValue(reset));
  face.addEventListener('keydown',e=>{let d=0;if(e.key==='ArrowUp'||e.key==='ArrowRight')d=step*(e.shiftKey?1:5);if(e.key==='ArrowDown'||e.key==='ArrowLeft')d=-step*(e.shiftKey?1:5);if(e.key==='Home'){e.preventDefault();return setValue(min);}if(e.key==='End'){e.preventDefault();return setValue(max);}if(d){e.preventDefault();setValue(Number(input.value)+d);}});
  input.addEventListener('input',sync);sync();
}

function fxControl(mod,name,param,min,max,format){
  const c=document.createElement('label');c.className='refm-fx-control';c.dataset.fxName=name;
  const value=mod._refm_wasm_get_fx(param);
  c.innerHTML=`<span>${name}</span><input type="range" min="${min}" max="${max}" value="${value}">`;
  const input=c.querySelector('input');input.oninput=e=>mod._refm_wasm_set_fx(param,+e.target.value);
  knobify(input,format);return c;
}

export function installFxBoard(mod){
  /* Keep the ACID panels stock-focused. These controls are not present on a
     TB-303 and were confusing the product concept. The ABI remains compatible
     with old projects, but the stock rack no longer exposes them. */
  document.querySelectorAll('.lfoRow,.refm-ext-toggle').forEach(x=>x.remove());
  document.querySelectorAll('.refm-stock-strip').forEach(strip=>{
    strip.querySelectorAll('span').forEach(s=>{if(/MOD|LFO/i.test(s.textContent))s.remove();});
  });
  document.querySelectorAll('.ctl label').forEach(l=>{if(l.textContent.trim()==='DRIVE')l.textContent='DISTORTION';});

  /* Split the single SEND control into real delay and reverb sends. */
  document.querySelectorAll('.refm-channel').forEach((channel,track)=>{
    const labels=[...channel.querySelectorAll('label')];
    const oldSend=labels.find(l=>l.textContent.trim().startsWith('SEND'));
    if(oldSend){const input=oldSend.querySelector('input');oldSend.childNodes[0].textContent='DELAY ';if(input)input.title='Delay send';}
    if(!channel.querySelector('[data-reverb-send]')){
      const label=document.createElement('label');label.innerHTML=`REVERB <input data-reverb-send type="range" min="0" max="127" value="${mod._refm_wasm_get_mix_track(track,5)}">`;
      label.querySelector('input').oninput=e=>mod._refm_wasm_set_mix_track(track,5,+e.target.value);
      channel.appendChild(label);
    }
  });

  const host=document.querySelector('.refm-fx-host');if(!host)return;
  host.innerHTML='';host.classList.add('refm-fx-board');
  const groups=[
    ['DISTORTION',[
      ['AMOUNT',0,0,20000,null]
    ]],
    ['DELAY',[
      ['TIME',5,64,2048,(v)=>`${Math.round(v/44.1)} ms`],
      ['FEEDBACK',3,0,30000,null],
      ['MIX',4,0,32767,null]
    ]],
    ['REVERB',[
      ['SIZE',6,0,31000,null],
      ['MIX',7,0,32767,null],
      ['DAMP',8,256,30000,null]
    ]],
    ['MASTER',[
      ['COMP',1,2048,32767,null],
      ['FILTER',2,256,32767,null]
    ]]
  ];
  groups.forEach(([title,defs])=>{
    const group=document.createElement('section');group.className='refm-fx-group';group.innerHTML=`<h3>${title}</h3><div class="refm-fx-group-controls"></div>`;
    const controls=group.querySelector('.refm-fx-group-controls');
    defs.forEach(([name,param,min,max,format])=>controls.appendChild(fxControl(mod,name,param,min,max,format)));
    host.appendChild(group);
  });
}
