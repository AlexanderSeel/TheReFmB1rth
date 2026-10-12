import { inspectPackage } from './fm1pkg.js';
import { Updater } from './fm1ota.js';

const $ = s => document.querySelector(s);
const $$ = s => [...document.querySelectorAll(s)];
const state = { access:null, updater:null, pkg:null, editorOut:null, machine:0, note:null };
const machineNames=['ACID A','ACID B','808','909'];
const channels=[0,1,2,3];

function log(msg){const el=$('#installLog');el.textContent += `\n${new Date().toLocaleTimeString()}  ${msg}`;el.scrollTop=el.scrollHeight;}
function setMidiStatus(text,on=false){const e=$('#midiStatus');e.textContent=text;e.classList.toggle('online',on);}
function updateUnlock(){const ok=!!state.pkg&&!!state.access&&$('#riskCheck').checked&&$('#confirmText').value.trim()==='FLASH THE FM-1';$('#installBtn').disabled=!ok;$('#resumeBtn').disabled=!ok;}
function fmtBytes(n){return n>1024*1024?`${(n/1024/1024).toFixed(2)} MB`:`${(n/1024).toFixed(1)} KB`;}

$$('.tab').forEach(b=>b.addEventListener('click',()=>{ $$('.tab').forEach(x=>x.classList.remove('active')); b.classList.add('active'); $$('.page').forEach(x=>x.classList.remove('active')); $('#'+b.dataset.tab).classList.add('active'); }));

async function ensureMidi(){
  if(!navigator.requestMIDIAccess) throw new Error('Web MIDI is unavailable. Use current Chrome or Edge on HTTPS/localhost.');
  if(!state.access){state.access=await navigator.requestMIDIAccess({sysex:true});state.updater=new Updater(state.access,log);state.access.onstatechange=()=>{if(state.editorOut?.state==='disconnected'){state.editorOut=null;setMidiStatus('MIDI DISCONNECTED');}};}
  return state.access;
}

async function loadPackage(bytes,source){
  const pkg=await inspectPackage(bytes);state.pkg=pkg;
  $('#pkgProduct').textContent=pkg.product;$('#pkgSize').textContent=fmtBytes(pkg.bytes);$('#pkgSha').textContent=pkg.sha256;
  log(`Package loaded from ${source}: ${pkg.product}, SHA-256 ${pkg.sha256}`);updateUnlock();
}

$('#fwFile').addEventListener('change',async e=>{const f=e.target.files?.[0];if(!f)return;try{await loadPackage(new Uint8Array(await f.arrayBuffer()),f.name);}catch(err){log(`PACKAGE ERROR: ${err.message}`);state.pkg=null;updateUnlock();}});
const drop=$('#dropZone');drop.addEventListener('dragover',e=>{e.preventDefault();drop.classList.add('drag');});drop.addEventListener('dragleave',()=>drop.classList.remove('drag'));drop.addEventListener('drop',async e=>{e.preventDefault();drop.classList.remove('drag');const f=e.dataTransfer.files?.[0];if(!f)return;try{await loadPackage(new Uint8Array(await f.arrayBuffer()),f.name);}catch(err){log(`PACKAGE ERROR: ${err.message}`);}});

$('#loadRelease').addEventListener('click',async()=>{
  const btn=$('#loadRelease');btn.disabled=true;btn.textContent='LOADING…';
  try{
    const rs=await fetch('https://api.github.com/repos/AlexanderSeel/TheReFmB1rth/releases?per_page=20',{headers:{Accept:'application/vnd.github+json'}});
    if(!rs.ok)throw new Error(`GitHub API ${rs.status}`);
    const releases=await rs.json();
    let found=null;
    for(const r of releases){if(r.draft)continue;const asset=r.assets?.find(a=>/\.fwsc$/i.test(a.name));if(asset){found={r,asset};break;}}
    if(!found)throw new Error('No GitHub Release containing a .fwsc asset was found.');
    log(`Downloading ${found.asset.name} from release ${found.r.tag_name}…`);
    const fw=await fetch(found.asset.browser_download_url);if(!fw.ok)throw new Error(`Firmware download ${fw.status}`);
    await loadPackage(new Uint8Array(await fw.arrayBuffer()),`GitHub ${found.r.tag_name}`);
  }catch(err){log(`RELEASE ERROR: ${err.message}`);}finally{btn.disabled=false;btn.textContent='LOAD LATEST GITHUB RELEASE';}
});

$('#connectInstall').addEventListener('click',async()=>{
  try{
    await ensureMidi();const info=await state.updater.identify();
    if(!info)throw new Error('No running FM-1 answered the identity handshake.');
    $('#deviceIdentity').textContent=info.text;$('#devicePort').textContent=info.name;$('#deviceLamp').classList.add('on');
    setMidiStatus(info.text,true);log(`Connected: ${info.text} on ${info.name}`);
  }catch(err){$('#deviceLamp').classList.remove('on');log(`CONNECT ERROR: ${err.message}`);}finally{updateUnlock();}
});
$('#riskCheck').addEventListener('change',updateUnlock);$('#confirmText').addEventListener('input',updateUnlock);

function installStep(kind,value){
  const labels={start:'Device verified; entering update mode',verify:'Staging/verifying loader',loader:'Loader started',write:'Writing firmware',reboot:'Rebooting',done:'Installation complete'};
  if(kind==='start')$('#progressBar').style.width='5%';
  else if(kind==='verify')$('#progressBar').style.width=`${Math.min(23,8+Number(value||0)*.4)}%`;
  else if(kind==='loader')$('#progressBar').style.width='25%';
  else if(kind==='write')$('#progressBar').style.width=`${Math.min(94,25+Number(value||0)*.18)}%`;
  else if(kind==='reboot')$('#progressBar').style.width='96%';
  else if(kind==='done')$('#progressBar').style.width='100%';
  if(kind!=='verify'&&kind!=='write')log(`${labels[kind]||kind}${value?`: ${value}`:''}`);
}

$('#installBtn').addEventListener('click',async()=>{
  if(!state.pkg)return;$('#installBtn').disabled=true;$('#resumeBtn').disabled=true;$('#progressBar').style.width='0';
  try{await state.updater.install(state.pkg.image,state.pkg.product,installStep);log('SUCCESS: firmware installed and identity verified after reboot.');}
  catch(err){log(`INSTALL ERROR [${err.code||'error'}]: ${err.message}`);if(err.code==='lost'||err.code==='noloader')log('If the OTA loader is active, reconnect USB and use RESUME OTA LOADER.');}
  finally{updateUnlock();}
});
$('#resumeBtn').addEventListener('click',async()=>{
  if(!state.pkg)return;$('#installBtn').disabled=true;$('#resumeBtn').disabled=true;
  try{const ok=await state.updater.resume(state.pkg.image,state.pkg.product,installStep);if(!ok)throw new Error('No matching OTA loader found.');log('SUCCESS: resumed loader write completed.');}
  catch(err){log(`RESUME ERROR: ${err.message}`);}finally{updateUnlock();}
});

function candidateEditorOutputs(access){return [...access.outputs.values()].filter(o=>o.state!=='disconnected'&&/fm-1|felucca|composite|sinco|usb-midi/i.test(o.name||''));}
$('#connectEditor').addEventListener('click',async()=>{
  try{
    const access=await ensureMidi();const outs=candidateEditorOutputs(access);
    if(outs.length!==1)throw new Error(outs.length?`Found ${outs.length} possible FM-1 outputs. Disconnect all but one.`:'No FM-1 MIDI output found.');
    state.editorOut=outs[0];await state.editorOut.open();setMidiStatus(`EDITOR · ${state.editorOut.name}`,true);$('#connectEditor').textContent='EDITOR CONNECTED';
  }catch(err){setMidiStatus('EDITOR OFFLINE');alert(err.message);}
});
function send(bytes){if(!state.editorOut) return false;try{state.editorOut.send(bytes);return true;}catch{return false;}}
function cc(num,val){return send([0xB0+channels[state.machine],num,Math.max(0,Math.min(127,Number(val)|0))]);}
function program(p){send([0xC0+channels[state.machine],p&7]);}

$$('.machine').forEach(b=>b.addEventListener('click',()=>{state.machine=Number(b.dataset.machine);$$('.machine').forEach(x=>x.classList.toggle('active',x===b));const acid=state.machine<2;$('#acidPanel').classList.toggle('hidden',!acid);$('#drumPanel').classList.toggle('hidden',acid);$('#acidName').textContent=machineNames[state.machine];$('#drumName').textContent=machineNames[state.machine];$('#mixTrackName').textContent=machineNames[state.machine];}));
$$('input[data-cc]').forEach(r=>r.addEventListener('input',()=>{r.nextElementSibling.textContent=r.value;cc(Number(r.dataset.cc),r.value);}));
$$('[data-toggle-cc]').forEach(b=>b.addEventListener('click',()=>{b.classList.toggle('active');cc(Number(b.dataset.toggleCc),b.classList.contains('active')?127:0);}));
$$('[data-wave]').forEach(b=>b.addEventListener('click',()=>{$$('[data-wave]').forEach(x=>x.classList.remove('active'));b.classList.add('active');cc(77,Number(b.dataset.wave)?127:0);}));

const notes=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];const keyboard=$('#keyboard');
for(let midi=36;midi<=59;midi++){const b=document.createElement('button');b.className='key '+(notes[midi%12].includes('#')?'black':'');b.textContent=`${notes[midi%12]}${Math.floor(midi/12)-1}`;const on=()=>{if(state.machine>1)return;state.note=midi;send([0x90+channels[state.machine],midi,100]);};const off=()=>{if(state.note===midi){send([0x80+channels[state.machine],midi,0]);state.note=null;}};b.addEventListener('pointerdown',on);b.addEventListener('pointerup',off);b.addEventListener('pointerleave',off);keyboard.appendChild(b);}

const patterns=$('#patterns');for(let i=0;i<8;i++){const b=document.createElement('button');b.textContent=String.fromCharCode(65+i);b.addEventListener('click',()=>{$$('#patterns button').forEach(x=>x.classList.remove('active'));b.classList.add('active');program(i);});patterns.appendChild(b);}patterns.firstElementChild.classList.add('active');
const drumMap=[['BD',36],['SD',38],['CP',39],['RS',37],['CH',42],['OH',46],['LT',41],['MT',45],['HT',48],['CR',49],['RD',51]];const drumPads=$('#drumPads');for(const [name,note] of drumMap){const b=document.createElement('button');b.textContent=name;b.addEventListener('pointerdown',()=>send([0x90+channels[state.machine],note,110]));b.addEventListener('pointerup',()=>send([0x80+channels[state.machine],note,0]));drumPads.appendChild(b);}

updateUnlock();
