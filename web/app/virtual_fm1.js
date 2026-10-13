// SPDX-License-Identifier: GPL-3.0-only
// Physical layout inspired by charlesvestal/fm1-x0x (GPL-3.0); audio/state/UI
// are TheReFmB1rth's own WASM + ui240 implementation.
const NS = 'http://www.w3.org/2000/svg';
const PAGES = ['HOME','ACID','STEP','DRUM','MIXER','FX','SONG','PROJECT','EXT'];
const MACHINES = ['ACID A','ACID B','808','909'];
const DRUMS = ['BD','SD','CP','RS','CH','OH','LT','MT','HT','CR','RD'];
const ACTION = {HOME:0,ENV:1,LFO:2,SEQ:3,ARP:4,GLO:5,FX:6,SEL:7,SAVE:8,PLAY:9,EDIT:10,REC:11};
const ENC = {K1:0,K2:1,K3:2,K4:3,PRESETS:4,ALGORITHM:5,SELECT:6};
const WHITE_K = [0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26];
const BLACK_K = [1,3,5,8,10,13,15,17,20,22,25];
const BLACK_AFTER = [0,1,2,4,5,7,8,9,11,12,14];
const MIDI_BASE = 36;
const STORE_KEY = 'refm-virtual-fm1-project-v1';

let mod = null;
let ctx = null;
let audioClient = null;
let ready = false;
let routeActive = false;
let projectResult = 'READY';

const root = document.querySelector('#virtualDevice');
const canvas = document.querySelector('#virtualScreen');
const screen = canvas?.getContext('2d');
const status = document.querySelector('#virtualStatus');
const cpu = document.querySelector('#virtualCpu');

function setStatus(s, on = false) {
  if (!status) return;
  status.textContent = s;
  status.classList.toggle('online', on);
}

async function loadModule() {
  if (mod) return mod;
  let createRefm = null;
  let last;
  let pathUsed = '';
  for (const path of ['../refm.js','../../build/wasm/refm.js']) {
    try {
      createRefm = (await import(path)).default;
      pathUsed = path;
      break;
    } catch (e) {
      last = e;
    }
  }
  if (!createRefm) throw new Error(`WASM module not found. Run web/emu/build_wasm.sh. ${last?.message || ''}`);
  const wasmUrl = pathUsed.startsWith('../refm.js')
    ? new URL('../refm.wasm', import.meta.url)
    : new URL('../../build/wasm/refm.wasm', import.meta.url);
  mod = await createRefm({locateFile:p => p.endsWith('.wasm') ? wasmUrl.href : p});
  mod._refm_wasm_init(128);
  ready = true;
  restoreLocal(false);
  drawScreen();
  return mod;
}

async function loadWorkletClient() {
  let last;
  for (const path of ['../worklet_client.js','../../build/wasm/worklet_client.js']) {
    try { return (await import(path)).ReFmWorkletClient; }
    catch (e) { last = e; }
  }
  throw new Error(`AudioWorklet client not found. ${last?.message || ''}`);
}

function midi(bytes) {
  if (!ready) return false;
  for (const b of bytes) mod._refm_wasm_midi(b);
  drawScreen();
  return true;
}

async function enableAudio() {
  await loadModule();
  if (ctx) {
    await ctx.resume();
    setStatus('VIRTUAL FM-1 · AUDIO ON', true);
    return;
  }
  ctx = new AudioContext({sampleRate:44100, latencyHint:'interactive'});
  const Client = await loadWorkletClient();
  audioClient = await new Client(ctx, mod, 512).init();
  window.addEventListener('refm-worklet-stats', e => {
    const d = e.detail || {};
    const audioMs = (d.rendered_frames || 0) / 44.1;
    const load = audioMs > 0 ? (d.render_ms || 0) / audioMs * 100 : 0;
    if (cpu) cpu.textContent = `browser DSP ${load.toFixed(1)}% · underflows ${d.underflows || 0}`;
  });
  await ctx.resume();
  setStatus('VIRTUAL FM-1 · AUDIOWORKLET ON', true);
}

function value(i) { return ready ? mod._refm_wasm_panel_value(i) >>> 0 : 0; }
function action(id) { if (ready) { mod._refm_wasm_panel_action(id); drawScreen(); } }
function encoder(id, d) { if (ready && d) { mod._refm_wasm_panel_encoder(id, d); drawScreen(); } }
function key(panelIndex, on) { if (ready) { mod._refm_wasm_panel_key(MIDI_BASE + panelIndex, on ? 1 : 0); drawScreen(); } }

function snapshotBytes() {
  if (!ready || mod._refm_wasm_snapshot() !== 0) return null;
  const n = mod._refm_wasm_snapshot_size();
  const p = mod._refm_wasm_snapshot_ptr();
  return new Uint8Array(mod.HEAPU8.slice(p, p + n));
}

function saveLocal() {
  const b = snapshotBytes();
  if (!b) return;
  localStorage.setItem(STORE_KEY, btoa(String.fromCharCode(...b)));
  projectResult = 'DONE';
  setStatus('VIRTUAL PROJECT SAVED', true);
  drawScreen();
}

function restoreLocal(show = true) {
  const s = localStorage.getItem(STORE_KEY);
  if (!s || !ready) {
    if (show) setStatus('NO VIRTUAL PROJECT SAVED', false);
    return;
  }
  try {
    const raw = atob(s);
    const b = Uint8Array.from(raw, c => c.charCodeAt(0));
    const p = mod._malloc(b.length);
    mod.HEAPU8.set(b, p);
    mod._refm_wasm_restore(p, b.length);
    mod._free(p);
    projectResult = 'DONE';
    if (show) setStatus('VIRTUAL PROJECT LOADED', true);
    drawScreen();
  } catch (e) {
    if (show) setStatus(`LOAD FAILED · ${e.message}`, false);
  }
}

function resetVirtual() {
  localStorage.removeItem(STORE_KEY);
  if (mod) mod._refm_wasm_init(128);
  projectResult = 'READY';
  drawScreen();
  setStatus('VIRTUAL FACTORY RESET', true);
}

// Mirrors tools/apply_rb240_skin.py. Keep geometry and semantic colors aligned.
const C = {
  bg:'#2b2e31', panel:'#707477', panel2:'#55595d', dark:'#202224', light:'#c0c4c6',
  led:'#250708', red:'#ff4a3f', amber:'#ffb326', cyan:'#38bfd2', green:'#6fd77d',
  text:'#f7f7f4', mid:'#c6c8ca'
};

function txt(x, y, s, size = 10, color = C.text, bold = false) {
  screen.fillStyle = color;
  screen.font = `${bold ? 'bold ' : ''}${size}px Arial Narrow,monospace`;
  screen.fillText(String(s), x, y);
}
function pct(v) { return Math.round((Number(v) * 100 + 16383) / 32767); }
function bevel(x, y, w, h) {
  screen.fillStyle = C.light;
  screen.fillRect(x, y, w, 2);
  screen.fillRect(x, y, 2, h);
  screen.fillStyle = C.dark;
  screen.fillRect(x, y + h - 2, w, 2);
  screen.fillRect(x + w - 2, y, 2, h);
}
function ledBox(x, y, w, h) {
  screen.fillStyle = C.led;
  screen.fillRect(x, y, w, h);
  screen.fillStyle = '#641619';
  screen.fillRect(x, y, w, 1);
}
function band(y, label, color) {
  screen.fillStyle = color;
  screen.fillRect(4, y, 232, 9);
  screen.fillStyle = C.light;
  screen.fillRect(4, y, 232, 1);
  screen.fillStyle = C.dark;
  screen.fillRect(4, y + 8, 232, 1);
  txt(9, y + 7, label, 7, C.text, true);
}
function meter(x, y, h, p, color) {
  const inner = h - 4;
  const clamped = Math.max(0, Math.min(100, Number(p) || 0));
  const fill = Math.floor(inner * clamped / 100);
  screen.fillStyle = C.panel;
  screen.fillRect(x, y, 12, h);
  screen.fillStyle = C.dark;
  screen.fillRect(x + 2, y + 2, 8, inner);
  if (fill > 0) {
    screen.fillStyle = color;
    screen.fillRect(x + 3, y + h - 2 - fill, 6, fill);
  }
}
function card(x, y, label, val) {
  screen.fillStyle = C.panel;
  screen.fillRect(x, y, 56, 44);
  bevel(x, y, 56, 44);
  txt(x + 5, y + 12, label, 8, C.text, true);
  ledBox(x + 5, y + 18, 46, 20);
  txt(x + 8, y + 34, val, 12, typeof val === 'number' ? C.red : C.amber, true);
}
function steps(selected, playing) {
  for (let i = 0; i < 16; i++) {
    const x = 8 + (i & 7) * 29;
    const y = 77 + (i >> 3) * 27;
    screen.fillStyle = C.panel;
    screen.fillRect(x, y, 22, 18);
    bevel(x, y, 22, 18);
    screen.fillStyle = i === selected ? C.amber : i === playing ? C.green : C.dark;
    screen.fillRect(x + 3, y + 3, 16, 12);
  }
}
function footer(s) {
  screen.fillStyle = C.panel2;
  screen.fillRect(0, 212, 240, 28);
  screen.fillStyle = C.light;
  screen.fillRect(0, 212, 240, 2);
  txt(5, 228, s, 8, C.text);
}

function drawScreen() {
  if (!screen) return;
  screen.imageSmoothingEnabled = false;
  screen.fillStyle = C.bg;
  screen.fillRect(0, 0, 240, 240);

  if (!ready) {
    screen.fillStyle = C.panel2;
    screen.fillRect(10, 84, 220, 68);
    bevel(10, 84, 220, 68);
    ledBox(22, 99, 196, 34);
    txt(31, 121, 'TheReFmB1rth', 13, C.red, true);
    txt(29, 144, 'SWITCH ON VIRTUAL FM-1', 8, C.text);
    return;
  }

  const page = value(0), machine = value(1), pattern = value(2), playing = value(3);
  const playStep = value(4), selStep = value(5), lane = value(6), slot = value(7), bpm = value(8);
  const p0 = value(9), p1 = value(10), p2 = value(11), p3 = value(12), wave = value(13);
  const stype = value(14), acc = value(15), slide = value(16), prob = value(17), dstate = value(18);
  const mute = value(19), solo = value(20);

  screen.fillStyle = C.panel2;
  screen.fillRect(0, 0, 240, 24);
  bevel(0, 0, 240, 24);
  txt(5, 16, PAGES[page] || 'REFM', 12, C.amber, true);
  txt(76, 16, MACHINES[machine] || '?', 12, C.text, true);
  ledBox(171, 4, 36, 16);
  txt(177, 16, bpm, 12, C.red, true);
  txt(213, 16, playing ? 'PLAY' : 'STOP', 8, playing ? C.green : C.mid, true);

  screen.fillStyle = C.panel;
  screen.fillRect(0, 27, 240, 22);
  bevel(0, 27, 240, 22);
  txt(5, 40, 'PATTERN', 8, C.text, true);
  ledBox(47, 30, 17, 15);
  txt(52, 41, String.fromCharCode(65 + (pattern & 7)), 8, C.red, true);
  txt(72, 40, 'PRESET  ALGO  SELECT', 6, C.mid);

  if (page === 0) {
    band(51, 'PERFORMANCE', C.amber);
    card(3,61,'TRACK',MACHINES[machine]);
    card(62,61,'PATTERN',String.fromCharCode(65 + (pattern & 7)));
    card(121,61,'STEP',playStep + 1);
    card(180,61,'STATE',playing ? 'PLAY' : 'STOP');
    screen.fillStyle = C.panel2;
    screen.fillRect(4,119,232,40);
    bevel(4,119,232,40);
    txt(8,137,'Keys play selected ACID track',10,C.text,true);
    footer('ENV=A  LFO=B  ARP=DRUM  SEQ=STEP');
  } else if (page === 1) {
    band(51, 'ACID CONTROL', C.amber);
    card(3,61,'CUTOFF',pct(p0));
    card(62,61,'RES',pct(p1));
    card(121,61,'ENV',pct(p2));
    card(180,61,'DECAY',pct(p3));
    card(3,112,'WAVE',wave ? 'SQUARE' : 'SAW');
    card(62,112,'EDIT','WAVE');
    card(121,112,'SEQ','STEP');
    card(180,112,'KEYS','LIVE');
    footer('K1 CUT K2 RES K3 ENV K4 DEC | EDIT WAVE');
  } else if (page === 2) {
    band(51, 'STEP PROGRAMMER', C.amber);
    steps(selStep, playing ? playStep : -1);
    card(3,139,'NOTE',p0);
    card(62,139,'TYPE',['REST','NOTE','TIE'][stype] || '?');
    card(121,139,'FLAGS',acc && slide ? 'A+S' : acc ? 'ACC' : slide ? 'SLIDE' : '--');
    card(180,139,'PROB',prob || p3);
    footer('PRESET STEP | K1 NOTE K2 TYPE K3 FLAGS K4 PROB');
  } else if (page === 3) {
    band(51, 'DRUM MACHINE', C.amber);
    steps(selStep, playing ? playStep : -1);
    card(3,139,'LANE',DRUMS[lane] || '?');
    card(62,139,'STATE',['OFF','HIT','ACC'][dstate] || '?');
    card(121,139,'STEP',selStep + 1);
    card(180,139,'KIT',machine === 2 ? '808' : '909');
    footer('K1 LANE K2 OFF/HIT/ACC | PRESET STEP');
  } else if (page === 4) {
    band(51, 'MIXER / SENDS', C.cyan);
    card(3,61,'LEVEL',pct(p0));
    card(62,61,'PAN',Math.round((Number(p1) - 32767) * 100 / 32767));
    card(121,61,'DELAY',Math.round(Number(p2) * 100 / 127));
    card(180,61,'REVERB',Math.round(Number(p3) * 100 / 127));
    card(3,112,'MUTE',mute ? 'ON' : 'OFF');
    card(62,112,'SOLO',solo ? 'ON' : 'OFF');
    card(121,112,'EDIT','MUTE');
    card(180,112,'REC','SOLO');
    meter(24,164,38,pct(p0),C.green);
    meter(83,164,38,Number(p1) * 100 / 65534,C.amber);
    meter(142,164,38,Number(p2) * 100 / 127,C.cyan);
    meter(201,164,38,Number(p3) * 100 / 127,C.cyan);
    footer('ALGO TRACK | K1 LVL K2 PAN K3 DLY K4 REV');
  } else if (page === 5) {
    band(51, 'MASTER FX', C.cyan);
    card(3,61,'DRIVE',pct(p0));
    card(62,61,'DLY FB',Math.round(Number(p1) * 100 / 31000));
    card(121,61,'REV MIX',pct(p2));
    card(180,61,'FILTER',pct(p3));
    meter(24,124,70,pct(p0),C.red);
    meter(83,124,70,Number(p1) * 100 / 31000,C.cyan);
    meter(142,124,70,pct(p2),C.cyan);
    meter(201,124,70,pct(p3),C.green);
    footer('K1 DRIVE K2 DELAY K3 REVERB K4 FILTER');
  } else if (page === 6) {
    band(51, 'PATTERN / SONG', C.amber);
    card(3,61,'MODE','PATTERN');
    card(62,61,'BANK','A-H');
    card(121,61,'PLAY',playing ? 'RUN' : 'STOP');
    card(180,61,'EDIT','LOOP');
    txt(8,137,'Pattern playback / chain focus',10,C.text,true);
    footer('PRESET PATTERN | PLAY START/STOP | EDIT LOOP');
  } else if (page === 7) {
    band(51, 'PROJECT MEMORY', C.amber);
    card(3,61,'SLOT',slot + 1);
    card(62,61,'OCT-','LOAD');
    card(121,61,'OCT+','SAVE');
    card(180,61,'STATUS',projectResult);
    txt(8,137,'PRESET or K1 selects project slot',10,C.text,true);
    footer('OCT- LOAD   OCT+ SAVE   HOME EXIT');
  } else {
    txt(8,92,'Reserved for beta diagnostics',10,C.text,true);
    footer('HOME returns to performance view');
  }
}

function svgEl(tag, a, p) {
  const e = document.createElementNS(NS, tag);
  for (const [k,v] of Object.entries(a)) e.setAttribute(k, v);
  (p || root.querySelector('svg')).appendChild(e);
  return e;
}
function label(g, x, y, t, s = 15) {
  const e = svgEl('text', {
    x, y, 'text-anchor':'middle', 'font-family':'Arial Narrow,Arial,sans-serif',
    'font-size':s, 'font-weight':'700', fill:'#e8e8ea', 'letter-spacing':'.05em'
  }, g);
  e.textContent = t;
  return e;
}

function buildPanel() {
  const svg = root.querySelector('svg');
  svg.innerHTML = '';
  // Keep the virtual chassis close to the real dark FM-1; the RB styling lives on its LCD.
  svgEl('rect',{x:4,y:4,width:992,height:632,rx:44,fill:'#3a3d42'},svg);

  function knob(x,y,r,name,id,master=false) {
    const g=svgEl('g',{class:'vf-knob',tabindex:0,'aria-label':name},svg);
    svgEl('circle',{cx:x,cy:y,r:r+5,fill:'#26282c'},g);
    svgEl('circle',{cx:x,cy:y,r,fill:'#1f2124',stroke:'#44474c','stroke-width':2},g);
    const ind=svgEl('line',{x1:x,y1:y-r+5,x2:x,y2:y-r+15,stroke:'#eee','stroke-width':3,'stroke-linecap':'round'},g);
    label(g,x,y-r-12,name,17);
    if (!master) {
      let drag=null,wheel=0,angle=0;
      const turn=n=>{
        if(!n)return;
        encoder(id,n);
        angle=(angle+n*18)%360;
        ind.setAttribute('transform',`rotate(${angle} ${x} ${y})`);
      };
      g.addEventListener('pointerdown',e=>{drag={y:e.clientY,acc:0};g.setPointerCapture(e.pointerId);});
      g.addEventListener('pointermove',e=>{if(!drag)return;drag.acc+=drag.y-e.clientY;drag.y=e.clientY;const n=Math.trunc(drag.acc/7);if(n){drag.acc-=n*7;turn(n);}});
      g.addEventListener('pointerup',()=>drag=null);
      g.addEventListener('wheel',e=>{e.preventDefault();const dy=e.deltaMode===1?e.deltaY*33:e.deltaY;if(Math.abs(dy)>=50){turn(dy<0?1:-1);wheel=0;return;}wheel-=dy;const n=Math.trunc(wheel/25);if(n){wheel-=n*25;turn(n);}});
      g.addEventListener('keydown',e=>{if(e.key==='ArrowUp'||e.key==='ArrowRight'){turn(1);e.preventDefault();}else if(e.key==='ArrowDown'||e.key==='ArrowLeft'){turn(-1);e.preventDefault();}});
    }
    return g;
  }

  knob(105,100,24,'MASTER',null,true);
  knob(215,100,24,'SELECT',ENC.SELECT);
  knob(105,205,24,'PRESETS',ENC.PRESETS);
  knob(215,205,24,'ALGORITHM',ENC.ALGORITHM);
  for(let i=0;i<4;i++) knob(610+i*104,100,22,`KNOB${i+1}`,i);

  function button(x,y,w,h,name,two) {
    const g=svgEl('g',{class:'vf-button',tabindex:0,'aria-label':name},svg);
    svgEl('rect',{x,y,width:w,height:h,rx:9,fill:'#2e3135',stroke:'#4a4d52','stroke-width':1.5,class:'vf-btnface'},g);
    if(two){label(g,x+w/2,y+h/2-1,name,12);label(g,x+w/2,y+h/2+12,two,12);}else label(g,x+w/2,y+h/2+5,name,12);
    const fire=()=>{
      if(name==='OCT-'){if(value(0)===7)restoreLocal();else encoder(ENC.ALGORITHM,-1);}
      else if(name==='OCT+'){if(value(0)===7)saveLocal();else encoder(ENC.ALGORITHM,1);}
      else action(ACTION[name]);
    };
    g.addEventListener('pointerdown',()=>{g.classList.add('held');fire();});
    g.addEventListener('pointerup',()=>g.classList.remove('held'));
    return g;
  }

  svgEl('rect',{x:58,y:258,width:210,height:58,rx:14,fill:'#26282c'},svg);
  button(72,268,82,38,'OCT-');
  button(170,268,82,38,'OCT+');
  svgEl('rect',{x:300,y:44,width:236,height:236,rx:26,fill:'#111214'},svg);
  svgEl('rect',{x:568,y:160,width:400,height:140,rx:18,fill:'#26282c'},svg);
  const r1=['FX','SEL','ENV','LFO','EDIT','GLO'],r2=['HOME','SAVE','ARP','SEQ','PLAY','REC'];
  for(let i=0;i<6;i++){
    button(582+i*63,174,54,48,r1[i]);
    button(582+i*63,236,54,48,r2[i],r2[i]==='PLAY'?'STOP':null);
  }

  const x0=36,y0=340,w=928,h=270,pitch=(w-24)/16,kw=pitch*.78;
  svgEl('rect',{x:x0,y:y0,width:w,height:h,rx:18,fill:'#26282c'},svg);
  function makeKey(panelIndex,x,y,hgt,black=false,labelText=''){
    const g=svgEl('g',{class:`vf-key${black?' black':''}`,'aria-label':labelText},svg);
    svgEl('rect',{x,y,width:kw,height:hgt,rx:kw/2,fill:black?'#c9cacd':'#d9dadc',class:'vf-keyface'},g);
    g.addEventListener('pointerdown',e=>{g.classList.add('held');g.setPointerCapture(e.pointerId);key(panelIndex,1);});
    const up=()=>{g.classList.remove('held');key(panelIndex,0);};
    g.addEventListener('pointerup',up);
    g.addEventListener('pointercancel',up);
    return g;
  }
  for(let i=0;i<16;i++) makeKey(WHITE_K[i],x0+12+i*pitch+(pitch-kw)/2,y0+h*.43,h*.52,false,`white key ${i+1}`);
  BLACK_AFTER.forEach((a,k)=>makeKey(BLACK_K[k],x0+12+(a+1)*pitch-kw/2,y0+h*.06,h*.33,true,`black key ${k+1}`));
  label(svg,160,325,'M-VAVE  FM-1',20);
  label(svg,792,329,'TheReFmB1rth',17);
}

function keyboardShortcuts(){
  const km={};
  'qwertyui'.split('').forEach((c,i)=>km[`Key${c.toUpperCase()}`]={key:WHITE_K[i]});
  'asdfghjk'.split('').forEach((c,i)=>km[`Key${c.toUpperCase()}`]={key:WHITE_K[8+i]});
  '1234567890'.split('').forEach((c,i)=>km[`Digit${c}`]={key:BLACK_K[i]});
  km.Minus={key:BLACK_K[10]};
  Object.assign(km,{Space:{action:ACTION.PLAY},Enter:{action:ACTION.REC},Escape:{action:ACTION.HOME},KeyZ:{oct:-1},KeyX:{oct:1},ArrowUp:{enc:1},ArrowDown:{enc:-1}});
  const held=new Map();
  window.addEventListener('keydown',e=>{
    if(!document.querySelector('#virtual')?.classList.contains('active')||e.repeat)return;
    const m=km[e.code];if(!m)return;e.preventDefault();
    if(m.key!==undefined){held.set(e.code,m.key);key(m.key,1);}
    else if(m.action!==undefined)action(m.action);
    else if(m.oct){if(value(0)===7)(m.oct<0?restoreLocal:saveLocal)();else encoder(ENC.ALGORITHM,m.oct);}
    else if(m.enc)encoder(ENC.SELECT,m.enc);
  });
  window.addEventListener('keyup',e=>{if(held.has(e.code)){key(held.get(e.code),0);held.delete(e.code);}});
  window.addEventListener('blur',()=>{for(const k of held.values())key(k,0);held.clear();});
}

function expose(){
  window.refmVirtual={
    get ready(){return ready;},
    get routeActive(){return routeActive;},
    set routeActive(v){
      routeActive=!!v;
      document.querySelector('#routeVirtual')?.classList.toggle('active',routeActive);
      setStatus(routeActive?'EDITOR ROUTED TO VIRTUAL FM-1':'VIRTUAL FM-1 READY',true);
      window.dispatchEvent(new CustomEvent('refm-virtual-route',{detail:{active:routeActive}}));
    },
    sendMidi:bytes=>midi(bytes),
    start:enableAudio,
    draw:drawScreen
  };
}

async function start(){
  const b=document.querySelector('#virtualPower');
  if(!b)return;
  b.disabled=true;
  try{await enableAudio();b.textContent='VIRTUAL FM-1 ON';drawScreen();}
  catch(e){setStatus(`VIRTUAL ERROR · ${e.message}`);b.disabled=false;}
}

function install(){
  if(!root||!canvas)return;
  buildPanel();
  drawScreen();
  keyboardShortcuts();
  expose();
  document.querySelector('#virtualPower')?.addEventListener('click',start);
  document.querySelector('#virtualSave')?.addEventListener('click',saveLocal);
  document.querySelector('#virtualReset')?.addEventListener('click',resetVirtual);
  document.querySelector('#routeVirtual')?.addEventListener('click',async()=>{if(!ready)await enableAudio();window.refmVirtual.routeActive=!routeActive;});
  setInterval(()=>{if(ready)drawScreen();},50);
}
install();