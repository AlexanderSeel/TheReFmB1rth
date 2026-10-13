// SPDX-License-Identifier: GPL-3.0-only
// Physical layout inspired by charlesvestal/fm1-x0x (GPL-3.0); audio/state/UI
// are TheReFmB1rth's own WASM + ui240 implementation.
import {drawRB240} from './rb240_renderer.js';

const NS='http://www.w3.org/2000/svg';
const ACTION={HOME:0,ENV:1,LFO:2,SEQ:3,ARP:4,GLO:5,FX:6,SEL:7,SAVE:8,PLAY:9,EDIT:10,REC:11};
const MOD={NONE:0,SEQ:1,DRUM:2,MIXER:3,FX:4,PATTERN:5,EDIT:6,LIVE:7,FOCUS:8};
const BUTTON_MOD={SEQ:1,ARP:2,GLO:3,FX:4,SAVE:5,EDIT:6,REC:7,SEL:8};
const MOD_PRIORITY=['SEQ','ARP','GLO','FX','SAVE','EDIT','REC','SEL'];
const ENC={K1:0,K2:1,K3:2,K4:3,PRESETS:4,ALGORITHM:5,SELECT:6};
const WHITE_K=[0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26];
const BLACK_K=[1,3,5,8,10,13,15,17,20,22,25];
const BLACK_AFTER=[0,1,2,4,5,7,8,9,11,12,14];
const PC_MOD_KEYS={KeyC:'SEQ',KeyV:'ARP',KeyB:'GLO',KeyN:'FX',KeyM:'SAVE',Comma:'EDIT',Period:'REC',Slash:'SEL'};
const MIDI_BASE=36,STORE_KEY='refm-virtual-fm1-project-v1',LONG_HOLD_MS=360;
let mod=null,ctx=null,audioClient=null,ready=false,routeActive=false,projectResult='READY';
const heldModifierButtons=new Map(),panelButtons=new Map();
let primaryModifierButton=null,modifierUsed=false;
const root=document.querySelector('#virtualDevice'),canvas=document.querySelector('#virtualScreen'),screen=canvas?.getContext('2d'),status=document.querySelector('#virtualStatus'),cpu=document.querySelector('#virtualCpu');

function setStatus(s,on=false){if(status){status.textContent=s;status.classList.toggle('online',on);}}
async function loadModule(){
  if(mod)return mod;
  let createRefm=null,last,pathUsed='';
  for(const path of ['../refm.js','../../build/wasm/refm.js']){try{createRefm=(await import(path)).default;pathUsed=path;break}catch(e){last=e;}}
  if(!createRefm)throw new Error(`WASM module not found. Run web/emu/build_wasm.sh. ${last?.message||''}`);
  const wasmUrl=pathUsed.startsWith('../refm.js')?new URL('../refm.wasm',import.meta.url):new URL('../../build/wasm/refm.wasm',import.meta.url);
  mod=await createRefm({locateFile:p=>p.endsWith('.wasm')?wasmUrl.href:p});
  mod._refm_wasm_init(128);ready=true;restoreLocal(false);drawScreen();return mod;
}
async function loadWorkletClient(){let last;for(const path of ['../worklet_client.js','../../build/wasm/worklet_client.js']){try{return (await import(path)).ReFmWorkletClient}catch(e){last=e;}}throw new Error(`AudioWorklet client not found. ${last?.message||''}`);}
function midi(bytes){if(!ready)return false;for(const b of bytes)mod._refm_wasm_midi(b);drawScreen();return true;}
async function enableAudio(){
  await loadModule();
  if(ctx){await ctx.resume();setStatus('VIRTUAL FM-1 · AUDIO ON',true);return;}
  ctx=new AudioContext({sampleRate:44100,latencyHint:'interactive'});
  const Client=await loadWorkletClient();audioClient=await new Client(ctx,mod,512).init();
  window.addEventListener('refm-worklet-stats',e=>{const d=e.detail||{},audioMs=(d.rendered_frames||0)/44.1,load=audioMs>0?(d.render_ms||0)/audioMs*100:0;if(cpu)cpu.textContent=`browser DSP ${load.toFixed(1)}% · underflows ${d.underflows||0}`;});
  await ctx.resume();setStatus('VIRTUAL FM-1 · AUDIOWORKLET ON',true);
}
function value(i){if(!ready)return 0;const f=mod._refm_wasm_panel_performance_value||mod._refm_wasm_panel_value;return f(i)>>>0;}
function modifier(id){if(ready&&mod._refm_wasm_panel_modifier)mod._refm_wasm_panel_modifier(id);}
function focusExit(){if(ready&&mod._refm_wasm_panel_focus_exit)mod._refm_wasm_panel_focus_exit();}
function activeModifierButton(){for(let i=MOD_PRIORITY.length-1;i>=0;i--)if(heldModifierButtons.has(MOD_PRIORITY[i]))return MOD_PRIORITY[i];return null;}
function syncModifier(){const name=activeModifierButton();modifier(name?BUTTON_MOD[name]:MOD.NONE);}
function hasHeldModifier(){return heldModifierButtons.size>0;}
function consumeModifier(){if(hasHeldModifier())modifierUsed=true;}
function clearModifiers(){for(const state of heldModifierButtons.values()){state.node?.classList.remove('held');state.node?.classList.remove('latched');}heldModifierButtons.clear();primaryModifierButton=null;modifierUsed=false;modifier(MOD.NONE);}
function beginKeyboardModifier(name,code){if(heldModifierButtons.has(name))return;if(!primaryModifierButton){primaryModifierButton=name;modifierUsed=false;}else if(primaryModifierButton!==name)modifierUsed=true;const node=panelButtons.get(name);node?.classList.add('held');heldModifierButtons.set(name,{node,pointerId:null,latched:false,unlatchOnUp:false,keyboardCode:code});syncModifier();setStatus(`VIRTUAL HOLD · ${name}`,true);drawScreen();}
function endKeyboardModifier(name,cancel=false){const state=heldModifierButtons.get(name);if(!state||!state.keyboardCode)return;const wasPrimary=primaryModifierButton===name,used=modifierUsed,tap=wasPrimary&&!cancel&&!used&&heldModifierButtons.size===1;heldModifierButtons.delete(name);state.node?.classList.remove('held');state.node?.classList.remove('latched');if(wasPrimary)primaryModifierButton=null;syncModifier();if(name==='SEL'&&tap)focusExit();if(tap)action(ACTION[name]);if(!heldModifierButtons.size)modifierUsed=false;drawScreen();}
function action(id){if(ready){const f=mod._refm_wasm_panel_performance_action||mod._refm_wasm_panel_action;f(id);drawScreen();}}
function encoder(id,d){if(ready&&d){consumeModifier();const f=mod._refm_wasm_panel_performance_encoder||mod._refm_wasm_panel_encoder;f(id,d);drawScreen();}}
function key(panelIndex,on){if(ready){if(on)consumeModifier();if(mod._refm_wasm_panel_key_event)mod._refm_wasm_panel_key_event(panelIndex,MIDI_BASE+panelIndex,on?1:0);else mod._refm_wasm_panel_key(MIDI_BASE+panelIndex,on?1:0);drawScreen();}}
function snapshotBytes(){if(!ready||mod._refm_wasm_snapshot()!==0)return null;const n=mod._refm_wasm_snapshot_size(),p=mod._refm_wasm_snapshot_ptr();return new Uint8Array(mod.HEAPU8.slice(p,p+n));}
function saveLocal(){const b=snapshotBytes();if(!b)return;localStorage.setItem(STORE_KEY,btoa(String.fromCharCode(...b)));projectResult='DONE';setStatus('VIRTUAL PROJECT SAVED',true);drawScreen();}
function restoreLocal(show=true){const s=localStorage.getItem(STORE_KEY);if(!s||!ready){if(show)setStatus('NO VIRTUAL PROJECT SAVED',false);return;}try{const raw=atob(s),b=Uint8Array.from(raw,c=>c.charCodeAt(0)),p=mod._malloc(b.length);mod.HEAPU8.set(b,p);mod._refm_wasm_restore(p,b.length);mod._free(p);projectResult='DONE';if(show)setStatus('VIRTUAL PROJECT LOADED',true);drawScreen();}catch(e){if(show)setStatus(`LOAD FAILED · ${e.message}`,false);}}
function resetVirtual(){localStorage.removeItem(STORE_KEY);if(mod)mod._refm_wasm_init(128);projectResult='READY';clearModifiers();drawScreen();setStatus('VIRTUAL FACTORY RESET',true);}
function drawScreen(){drawRB240(screen,k=>k==='ready'?ready:value(k),projectResult);}

function svgEl(tag,a,p){const e=document.createElementNS(NS,tag);for(const[k,v]of Object.entries(a))e.setAttribute(k,v);(p||root.querySelector('svg')).appendChild(e);return e;}
function label(g,x,y,t,s=15){const e=svgEl('text',{x,y,'text-anchor':'middle','font-family':'Arial Narrow,Arial,sans-serif','font-size':s,'font-weight':'700',fill:'#e8e8ea','letter-spacing':'.05em'},g);e.textContent=t;return e;}
function buildPanel(){
  const svg=root.querySelector('svg');svg.innerHTML='';
  svgEl('rect',{x:4,y:4,width:992,height:632,rx:44,fill:'#3a3d42'},svg);
  function knob(x,y,r,name,id,master=false){const g=svgEl('g',{class:'vf-knob',tabindex:0,'aria-label':name},svg);svgEl('circle',{cx:x,cy:y,r:r+5,fill:'#26282c'},g);svgEl('circle',{cx:x,cy:y,r,fill:'#1f2124',stroke:'#44474c','stroke-width':2},g);const ind=svgEl('line',{x1:x,y1:y-r+5,x2:x,y2:y-r+15,stroke:'#eee','stroke-width':3,'stroke-linecap':'round'},g);label(g,x,y-r-12,name,17);if(!master){let drag=null,wheel=0,angle=0;const turn=n=>{if(!n)return;encoder(id,n);angle=(angle+n*18)%360;ind.setAttribute('transform',`rotate(${angle} ${x} ${y})`);};g.addEventListener('pointerdown',e=>{drag={y:e.clientY,acc:0};g.setPointerCapture(e.pointerId)});g.addEventListener('pointermove',e=>{if(!drag)return;drag.acc+=drag.y-e.clientY;drag.y=e.clientY;const n=Math.trunc(drag.acc/7);if(n){drag.acc-=n*7;turn(n)}});g.addEventListener('pointerup',()=>drag=null);g.addEventListener('wheel',e=>{e.preventDefault();const dy=e.deltaMode===1?e.deltaY*33:e.deltaY;if(Math.abs(dy)>=50){turn(dy<0?1:-1);wheel=0;return;}wheel-=dy;const n=Math.trunc(wheel/25);if(n){wheel-=n*25;turn(n)}});g.addEventListener('keydown',e=>{if(e.key==='ArrowUp'||e.key==='ArrowRight'){turn(1);e.preventDefault();}else if(e.key==='ArrowDown'||e.key==='ArrowLeft'){turn(-1);e.preventDefault();}});}return g;}
  knob(105,100,24,'MASTER',null,true);knob(215,100,24,'SELECT',ENC.SELECT);knob(105,205,24,'PRESETS',ENC.PRESETS);knob(215,205,24,'ALGORITHM',ENC.ALGORITHM);for(let i=0;i<4;i++)knob(610+i*104,100,22,`KNOB${i+1}`,i);
  function button(x,y,w,h,name,two){const g=svgEl('g',{class:'vf-button',tabindex:0,'aria-label':name},svg);panelButtons.set(name,g);svgEl('rect',{x,y,width:w,height:h,rx:9,fill:'#2e3135',stroke:'#4a4d52','stroke-width':1.5,class:'vf-btnface'},g);if(two){label(g,x+w/2,y+h/2-1,name,12);label(g,x+w/2,y+h/2+12,two,12);}else label(g,x+w/2,y+h/2+5,name,12);const fire=()=>{if(name==='OCT-'){if(value(0)===7)restoreLocal();else encoder(ENC.ALGORITHM,-1);}else if(name==='OCT+'){if(value(0)===7)saveLocal();else encoder(ENC.ALGORITHM,1);}else action(ACTION[name]);};let holdTimer=null;const down=e=>{g.classList.add('held');if(BUTTON_MOD[name]!==undefined){const existing=heldModifierButtons.get(name);if(existing?.latched){existing.unlatchOnUp=true;existing.pointerId=e.pointerId;if(g.setPointerCapture)g.setPointerCapture(e.pointerId);return;}if(!primaryModifierButton){primaryModifierButton=name;modifierUsed=false;}else if(primaryModifierButton!==name)modifierUsed=true;const state={node:g,pointerId:e.pointerId,latched:false,unlatchOnUp:false};heldModifierButtons.set(name,state);syncModifier();holdTimer=setTimeout(()=>{const current=heldModifierButtons.get(name);if(current&&current.pointerId===e.pointerId){current.latched=true;g.classList.add('latched');setStatus(`VIRTUAL HOLD · ${name} LATCHED`,true);}},LONG_HOLD_MS);if(g.setPointerCapture)g.setPointerCapture(e.pointerId);return;}consumeModifier();fire();};const finish=(e,cancel=false)=>{if(holdTimer){clearTimeout(holdTimer);holdTimer=null;}if(BUTTON_MOD[name]!==undefined){const state=heldModifierButtons.get(name);if(state){if(state.unlatchOnUp){heldModifierButtons.delete(name);g.classList.remove('held');g.classList.remove('latched');if(primaryModifierButton===name)primaryModifierButton=null;syncModifier();if(!heldModifierButtons.size)modifierUsed=false;setStatus('VIRTUAL HOLD RELEASED',true);drawScreen();}else if(state.latched&&!cancel){state.pointerId=null;g.classList.add('held');g.classList.add('latched');drawScreen();}else{const wasPrimary=primaryModifierButton===name,used=modifierUsed,tap=wasPrimary&&!cancel&&!used&&heldModifierButtons.size===1;heldModifierButtons.delete(name);g.classList.remove('held');g.classList.remove('latched');if(wasPrimary)primaryModifierButton=null;syncModifier();if(name==='SEL'&&tap)focusExit();if(tap)fire();if(!heldModifierButtons.size)modifierUsed=false;drawScreen();}}else g.classList.remove('held');}else g.classList.remove('held');if(e&&g.hasPointerCapture&&g.hasPointerCapture(e.pointerId))g.releasePointerCapture(e.pointerId);};g.addEventListener('pointerdown',down);g.addEventListener('pointerup',e=>finish(e,false));g.addEventListener('pointercancel',e=>finish(e,true));return g;}
  svgEl('rect',{x:58,y:258,width:210,height:58,rx:14,fill:'#26282c'},svg);button(72,268,82,38,'OCT-');button(170,268,82,38,'OCT+');svgEl('rect',{x:300,y:44,width:236,height:236,rx:26,fill:'#111214'},svg);svgEl('rect',{x:568,y:160,width:400,height:140,rx:18,fill:'#26282c'},svg);
  const r1=['FX','SEL','ENV','LFO','EDIT','GLO'],r2=['HOME','SAVE','ARP','SEQ','PLAY','REC'];for(let i=0;i<6;i++){button(582+i*63,174,54,48,r1[i]);button(582+i*63,236,54,48,r2[i],r2[i]==='PLAY'?'STOP':null);}
  const x0=36,y0=340,w=928,h=270,pitch=(w-24)/16,kw=pitch*.78;svgEl('rect',{x:x0,y:y0,width:w,height:h,rx:18,fill:'#26282c'},svg);
  function makeKey(panelIndex,x,y,hgt,black=false,labelText=''){const g=svgEl('g',{class:`vf-key${black?' black':''}`,'aria-label':labelText},svg);svgEl('rect',{x,y,width:kw,height:hgt,rx:kw/2,fill:black?'#c9cacd':'#d9dadc',class:'vf-keyface'},g);g.addEventListener('pointerdown',e=>{g.classList.add('held');g.setPointerCapture(e.pointerId);key(panelIndex,1);});const up=()=>{g.classList.remove('held');key(panelIndex,0);};g.addEventListener('pointerup',up);g.addEventListener('pointercancel',up);return g;}
  for(let i=0;i<16;i++)makeKey(WHITE_K[i],x0+12+i*pitch+(pitch-kw)/2,y0+h*.43,h*.52,false,`white key ${i+1}`);BLACK_AFTER.forEach((a,k)=>makeKey(BLACK_K[k],x0+12+(a+1)*pitch-kw/2,y0+h*.06,h*.33,true,`black key ${k+1}`));label(svg,160,325,'M-VAVE  FM-1',20);label(svg,792,329,'TheReFmB1rth',17);
}
function keyboardShortcuts(){const km={};'qwertyui'.split('').forEach((c,i)=>km[`Key${c.toUpperCase()}`]={key:WHITE_K[i]});'asdfghjk'.split('').forEach((c,i)=>km[`Key${c.toUpperCase()}`]={key:WHITE_K[8+i]});'1234567890'.split('').forEach((c,i)=>km[`Digit${c}`]={key:BLACK_K[i]});km.Minus={key:BLACK_K[10]};Object.assign(km,{Space:{action:ACTION.PLAY},Enter:{action:ACTION.REC},Escape:{action:ACTION.HOME},KeyZ:{oct:-1},KeyX:{oct:1},ArrowUp:{enc:1},ArrowDown:{enc:-1}});const held=new Map();window.addEventListener('keydown',e=>{if(!document.querySelector('#virtual')?.classList.contains('active')||e.repeat)return;const modName=PC_MOD_KEYS[e.code];if(modName){e.preventDefault();held.set(e.code,{modifier:modName});beginKeyboardModifier(modName,e.code);return;}const m=km[e.code];if(!m)return;e.preventDefault();if(m.key!==undefined){held.set(e.code,{key:m.key});key(m.key,1);}else if(m.action!==undefined){consumeModifier();action(m.action);}else if(m.oct){consumeModifier();if(value(0)===7)(m.oct<0?restoreLocal:saveLocal)();else encoder(ENC.ALGORITHM,m.oct);}else if(m.enc)encoder(ENC.SELECT,m.enc);});window.addEventListener('keyup',e=>{const h=held.get(e.code);if(!h)return;if(h.modifier)endKeyboardModifier(h.modifier,false);else if(h.key!==undefined)key(h.key,0);held.delete(e.code);});window.addEventListener('blur',()=>{for(const h of held.values())if(h.key!==undefined)key(h.key,0);held.clear();clearModifiers();});}
function expose(){window.refmVirtual={get ready(){return ready;},get routeActive(){return routeActive;},set routeActive(v){routeActive=!!v;document.querySelector('#routeVirtual')?.classList.toggle('active',routeActive);setStatus(routeActive?'EDITOR ROUTED TO VIRTUAL FM-1':'VIRTUAL FM-1 READY',true);window.dispatchEvent(new CustomEvent('refm-virtual-route',{detail:{active:routeActive}}));},sendMidi:bytes=>midi(bytes),start:enableAudio,draw:drawScreen};}
async function start(){const b=document.querySelector('#virtualPower');if(!b)return;b.disabled=true;try{await enableAudio();b.textContent='VIRTUAL FM-1 ON';drawScreen();}catch(e){setStatus(`VIRTUAL ERROR · ${e.message}`);b.disabled=false;}}
function install(){if(!root||!canvas)return;buildPanel();drawScreen();keyboardShortcuts();expose();document.querySelector('#virtualPower')?.addEventListener('click',start);document.querySelector('#virtualSave')?.addEventListener('click',saveLocal);document.querySelector('#virtualReset')?.addEventListener('click',resetVirtual);document.querySelector('#routeVirtual')?.addEventListener('click',async()=>{if(!ready)await enableAudio();window.refmVirtual.routeActive=!routeActive;});setInterval(()=>{if(ready)drawScreen();},50);}
install();
