// SPDX-License-Identifier: GPL-3.0-only
const NS='http://www.w3.org/2000/svg';
const PAGES=['HOME','ACID','STEP','DRUM','MIXER','FX','SONG','PROJECT','EXT'];
const MACHINES=['ACID A','ACID B','808','909'];
const ACTION={HOME:0,ENV:1,LFO:2,SEQ:3,ARP:4,GLO:5,FX:6,SEL:7,SAVE:8,PLAY:9,EDIT:10,REC:11,PANIC:14};
const ENC={K1:0,K2:1,K3:2,K4:3,PRESETS:4,ALGORITHM:5,SELECT:6};
const STORE_KEY='refm-virtual-fm1-project-v1';
let mod=null,ctx=null,node=null,ptr=0,ready=false,routeActive=false,lastFrame=0;
const root=document.querySelector('#virtualDevice');
const canvas=document.querySelector('#virtualScreen');
const screen=canvas?.getContext('2d');
const status=document.querySelector('#virtualStatus');
const cpu=document.querySelector('#virtualCpu');

function setStatus(s,on=false){if(status){status.textContent=s;status.classList.toggle('online',on);}}
async function loadModule(){
  if(mod)return mod;
  let createRefm=null,last;
  for(const path of ['../refm.js','../../build/wasm/refm.js']){try{createRefm=(await import(path)).default;break}catch(e){last=e;}}
  if(!createRefm)throw new Error(`WASM module not found. Build web/emu/build_wasm.sh first. ${last?.message||''}`);
  mod=await createRefm({locateFile:p=>p.endsWith('.wasm')?new URL('../refm.wasm',import.meta.url).href:p});
  mod._refm_wasm_init(128);ready=true;restoreLocal();drawScreen();return mod;
}
function midi(bytes){if(!ready)return false;for(const b of bytes)mod._refm_wasm_midi(b);drawScreen();return true;}
async function enableAudio(){
  await loadModule();
  if(ctx){await ctx.resume();setStatus('VIRTUAL FM-1 · AUDIO ON',true);return;}
  ctx=new AudioContext({sampleRate:44100});const frames=1024;ptr=mod._malloc(frames*8);node=ctx.createScriptProcessor(frames,0,2);
  let work=0,total=0;
  node.onaudioprocess=e=>{const t=performance.now();mod._refm_wasm_render(ptr,frames);const H=mod.HEAP32,base=ptr>>2,L=e.outputBuffer.getChannelData(0),R=e.outputBuffer.getChannelData(1);for(let i=0;i<frames;i++){L[i]=H[base+i*2]/32768;R[i]=H[base+i*2+1]/32768;}work+=performance.now()-t;total+=frames/44.1;if(total>1000){if(cpu)cpu.textContent=`browser DSP ${(work/total*100).toFixed(1)}%`;work=0;total=0;}};
  node.connect(ctx.destination);await ctx.resume();setStatus('VIRTUAL FM-1 · AUDIO ON',true);
}
function value(i){return ready?mod._refm_wasm_panel_value(i)>>>0:0;}
function action(id){if(!ready)return;mod._refm_wasm_panel_action(id);drawScreen();}
function encoder(id,d){if(!ready)return;mod._refm_wasm_panel_encoder(id,d);drawScreen();}
function key(note,on){if(!ready)return;mod._refm_wasm_panel_key(note,on?1:0);drawScreen();}
function snapshotBytes(){if(!ready||mod._refm_wasm_snapshot()!==0)return null;const n=mod._refm_wasm_snapshot_size(),p=mod._refm_wasm_snapshot_ptr();return new Uint8Array(mod.HEAPU8.slice(p,p+n));}
function saveLocal(){const b=snapshotBytes();if(!b)return;localStorage.setItem(STORE_KEY,btoa(String.fromCharCode(...b)));setStatus('VIRTUAL PROJECT SAVED',true);}
function restoreLocal(){const s=localStorage.getItem(STORE_KEY);if(!s||!ready)return;try{const raw=atob(s),b=new Uint8Array(raw.length);for(let i=0;i<raw.length;i++)b[i]=raw.charCodeAt(i);const p=mod._malloc(b.length);mod.HEAPU8.set(b,p);mod._refm_wasm_restore(p,b.length);mod._free(p);}catch{}}
function resetVirtual(){localStorage.removeItem(STORE_KEY);if(mod)mod._refm_wasm_init(128);drawScreen();setStatus('VIRTUAL FACTORY RESET',true);}

function drawScreen(){
  if(!screen)return;screen.imageSmoothingEnabled=false;screen.fillStyle='#07110d';screen.fillRect(0,0,240,240);
  if(!ready){screen.fillStyle='#65ff9b';screen.font='bold 13px monospace';screen.fillText('TheReFmB1rth',18,105);screen.font='10px monospace';screen.fillText('SWITCH ON VIRTUAL FM-1',18,126);return;}
  const page=value(0),machine=value(1),pattern=value(2),playing=value(3),playStep=value(4),selStep=value(5),lane=value(6),slot=value(7),bpm=value(8),p=[value(9),value(10),value(11),value(12)];
  screen.fillStyle='#7dff9f';screen.font='bold 12px monospace';screen.fillText(PAGES[page]||'?',8,16);screen.textAlign='right';screen.fillText(`${MACHINES[machine]}  ${String.fromCharCode(65+pattern)}`,232,16);screen.textAlign='left';
  screen.strokeStyle='#245838';screen.strokeRect(5,23,230,1);screen.fillStyle='#dfffe8';screen.font='bold 26px monospace';screen.fillText(`${bpm}`,9,53);screen.font='10px monospace';screen.fillStyle='#75bc89';screen.fillText('BPM',70,50);screen.fillStyle=playing?'#79ff96':'#52705b';screen.fillText(playing?'PLAY':'STOP',190,50);
  if(page===0){screen.fillStyle='#9be7b1';screen.font='11px monospace';screen.fillText('PRESET pattern   ALGO machine',9,82);screen.fillText('SELECT tempo     PLAY transport',9,99);screen.fillText('ENV A  LFO B  ARP drums  SEQ step',9,116);drawSteps(playStep,selStep,pattern);}
  else if(page===1){drawParamBoxes(['CUTOFF','RESO','ENV MOD','DECAY'],p);screen.fillStyle='#9be7b1';screen.font='10px monospace';screen.fillText(value(13)?'SQUARE · EDIT toggles wave':'SAW · EDIT toggles wave',9,180);drawSteps(playStep,selStep,pattern);}
  else if(page===2){const types=['REST','NOTE','TIE'];drawParamBoxes(['NOTE','TYPE','ACC/SLD','PROB'],[p[0],types[value(14)]||'?',`${value(15)?'A':'-'}${value(16)?'S':'-'}`,`${value(17)}%`]);drawSteps(playStep,selStep,pattern);}
  else if(page===3){drawParamBoxes(['LANE','STATE','STEP','MODEL'],[lane+1,['OFF','HIT','ACC'][value(18)]||'?',selStep+1,MACHINES[machine]]);drawSteps(playStep,selStep,pattern);}
  else if(page===4){drawParamBoxes(['LEVEL','PAN','DELAY','REVERB'],p);screen.fillStyle='#9be7b1';screen.font='10px monospace';screen.fillText(`EDIT MUTE ${value(19)?'ON':'OFF'} · REC SOLO ${value(20)?'ON':'OFF'}`,9,180);}
  else if(page===5)drawParamBoxes(['DRIVE','DLY FB','REV MIX','FILTER'],p);
  else if(page===6){screen.fillStyle='#dfffe8';screen.font='bold 20px monospace';screen.fillText(`PATTERN ${String.fromCharCode(65+pattern)}`,12,92);screen.font='11px monospace';screen.fillText('EDIT toggles song loop',12,118);}
  else if(page===7){screen.fillStyle='#dfffe8';screen.font='bold 24px monospace';screen.fillText(`PROJECT ${slot+1}`,18,91);screen.font='11px monospace';screen.fillText('PRESETS selects slot',18,117);screen.fillText('SAVE stores virtual browser state',18,135);}
  screen.fillStyle='#163222';screen.fillRect(0,211,240,29);screen.fillStyle='#a7d9b5';screen.font='9px monospace';screen.fillText(footer(page,machine),7,229);
}
function drawParamBoxes(labels,vals){for(let i=0;i<4;i++){const x=7+i*58;screen.fillStyle='#10281a';screen.fillRect(x,72,53,78);screen.strokeStyle='#2a6140';screen.strokeRect(x+.5,72.5,52,77);screen.fillStyle='#78b98c';screen.font='8px monospace';screen.fillText(labels[i],x+4,88);screen.fillStyle='#eaffef';screen.font='bold 12px monospace';const v=typeof vals[i]==='number'&&vals[i]>999?Math.round(vals[i]/258):vals[i];screen.fillText(String(v),x+4,116);}}
function drawSteps(play,sel){for(let i=0;i<16;i++){const x=8+(i%8)*28,y=157+Math.floor(i/8)*22;screen.fillStyle=i===play?'#68ff8b':i===sel?'#d9f5df':'#244833';screen.fillRect(x,y,22,16);screen.fillStyle=i===play?'#07110d':'#a7d9b5';screen.font='8px monospace';screen.fillText(String(i+1).padStart(2,'0'),x+4,y+11);}}
function footer(page,m){if(page===1)return'K1 CUT  K2 RES  K3 ENV  K4 DEC';if(page===2)return'K1 NOTE K2 TYPE K3 FLAGS K4 PROB';if(page===3)return'K1 LANE K2 STATE · PRESET STEP';if(page===4)return'K1 LVL K2 PAN K3 DLY K4 REV';if(page===5)return'K1 DRV K2 DLY K3 REV K4 FLT';if(page===7)return'PRESET SLOT · SAVE PROJECT';return`${MACHINES[m]} · PRESET PAT · ALGO MACHINE`;}

function svgEl(tag,a,p){const e=document.createElementNS(NS,tag);for(const[k,v]of Object.entries(a))e.setAttribute(k,v);(p||root.querySelector('svg')).appendChild(e);return e;}
function label(g,x,y,t,s=16){const e=svgEl('text',{x,y,'text-anchor':'middle','font-family':'Arial Narrow,Arial,sans-serif','font-size':s,'font-weight':'700',fill:'#e8e8ea'},g);e.textContent=t;return e;}
function buildPanel(){
  const svg=root.querySelector('svg');svg.innerHTML='';svgEl('rect',{x:4,y:4,width:992,height:632,rx:44,fill:'#3a3d42'},svg);svgEl('rect',{x:288,y:36,width:260,height:250,rx:14,fill:'#22252a',stroke:'#555','stroke-width':3},svg);
  const knob=(x,y,name,id,master=false)=>{const g=svgEl('g',{'data-enc':id??'',class:'vf-knob',tabindex:0},svg);svgEl('circle',{cx:x,cy:y,r:31,fill:'#25272a',stroke:'#101113','stroke-width':8},g);svgEl('line',{x1:x,y1:y-22,x2:x,y2:y-9,stroke:'#eee','stroke-width':3,'stroke-linecap':'round'},g);label(g,x,y-42,name,14);if(!master){g.addEventListener('wheel',e=>{e.preventDefault();encoder(id,e.deltaY<0?1:-1)});let sy=null;g.addEventListener('pointerdown',e=>{sy=e.clientY;g.setPointerCapture(e.pointerId)});g.addEventListener('pointermove',e=>{if(sy===null)return;const dy=sy-e.clientY;if(Math.abs(dy)>=8){encoder(id,dy>0?1:-1);sy=e.clientY}});g.addEventListener('pointerup',()=>sy=null);}return g;};
  knob(104,95,'MASTER',null,true);knob(215,95,'SELECT',ENC.SELECT);knob(104,195,'PRESETS',ENC.PRESETS);knob(215,195,'ALGORITHM',ENC.ALGORITHM);for(let i=0;i<4;i++)knob(625+i*103,96,`KNOB ${i+1}`,i);
  const btns=[['FX',590,190,ACTION.FX],['SEL',660,190,ACTION.SEL],['ENV',730,190,ACTION.ENV],['LFO',800,190,ACTION.LFO],['EDIT',870,190,ACTION.EDIT],['GLO',590,245,ACTION.GLO],['HOME',660,245,ACTION.HOME],['SAVE',730,245,ACTION.SAVE],['ARP',800,245,ACTION.ARP],['SEQ',870,245,ACTION.SEQ],['PLAY',590,310,ACTION.PLAY],['REC',670,310,ACTION.REC],['OCT -',780,310,ACTION.PANIC],['OCT +',860,310,ACTION.PANIC]];
  for(const [name,x,y,a] of btns){const g=svgEl('g',{class:'vf-button','data-action':a},svg);svgEl('rect',{x,y,width:62,height:38,rx:7,fill:'#2b2e32',stroke:'#555','stroke-width':2,class:'vf-btnface'},g);label(g,x+31,y+24,name,12);g.addEventListener('pointerdown',()=>{g.classList.add('held');if(name==='SAVE'&&value(0)===7)saveLocal();else action(a)});g.addEventListener('pointerup',()=>g.classList.remove('held'));}
  const whiteX=55,whiteY=430,ww=54,wh=165;for(let i=0;i<16;i++){const x=whiteX+i*ww;const g=svgEl('g',{class:'vf-key'},svg);svgEl('rect',{x,y:whiteY,width:ww-3,height:wh,rx:4,fill:'#dedfe1',stroke:'#8b8d90','stroke-width':2,class:'vf-keyface'},g);label(g,x+ww/2-2,whiteY+145,String(i+1),12);const note=36+i;g.addEventListener('pointerdown',e=>{g.classList.add('held');g.setPointerCapture(e.pointerId);key(note,1)});g.addEventListener('pointerup',()=>{g.classList.remove('held');key(note,0)});}
  const blackAfter=[0,1,3,4,5,7,8,10,11,12,14];for(let i=0;i<blackAfter.length;i++){const x=whiteX+(blackAfter[i]+1)*ww-17;const g=svgEl('g',{class:'vf-key black'},svg);svgEl('rect',{x,y:whiteY,width:32,height:94,rx:3,fill:'#242629',stroke:'#090a0b','stroke-width':2,class:'vf-keyface'},g);const note=37+i;g.addEventListener('pointerdown',e=>{g.classList.add('held');g.setPointerCapture(e.pointerId);key(note,1)});g.addEventListener('pointerup',()=>{g.classList.remove('held');key(note,0)});}
  label(svg,120,365,'M-VAVE FM-1',22);label(svg,800,390,'TheReFmB1rth',18);
}

function keyboardShortcuts(){const map={Escape:ACTION.HOME,Space:ACTION.PLAY,Enter:ACTION.REC};window.addEventListener('keydown',e=>{if(document.querySelector('#virtual')?.classList.contains('active')&&map[e.code]!=null){e.preventDefault();action(map[e.code]);}});}
function expose(){window.refmVirtual={get ready(){return ready},get routeActive(){return routeActive},set routeActive(v){routeActive=!!v;document.querySelector('#routeVirtual')?.classList.toggle('active',routeActive);setStatus(routeActive?'EDITOR ROUTED TO VIRTUAL FM-1':'VIRTUAL FM-1 READY',true)},sendMidi:bytes=>midi(bytes),start:enableAudio,draw:drawScreen};}
async function start(){const b=document.querySelector('#virtualPower');b.disabled=true;try{await enableAudio();b.textContent='VIRTUAL FM-1 ON';drawScreen();}catch(e){setStatus(`VIRTUAL ERROR · ${e.message}`);b.disabled=false;}}
function install(){if(!root||!canvas)return;buildPanel();drawScreen();keyboardShortcuts();expose();document.querySelector('#virtualPower')?.addEventListener('click',start);document.querySelector('#virtualSave')?.addEventListener('click',saveLocal);document.querySelector('#virtualReset')?.addEventListener('click',resetVirtual);document.querySelector('#routeVirtual')?.addEventListener('click',async()=>{if(!ready)await enableAudio();window.refmVirtual.routeActive=!routeActive;});setInterval(()=>{if(ready)drawScreen()},50);}
install();
