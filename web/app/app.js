import { inspectPackage } from './fm1pkg.js';
import { Updater } from './fm1ota.js';
import { installReadback } from './readback.js';

const $ = s => document.querySelector(s);
const $$ = s => [...document.querySelectorAll(s)];
const machineNames=['ACID A','ACID B','808','909'];
const channels=[0,1,2,3];
const drumMap=[['BD',36],['SD',38],['CP',39],['RS',37],['CH',42],['OH',46],['LT',41],['MT',45],['HT',48],['CR',49],['RD',51]];
const RELEASE_BY_TAG_API='https://api.github.com/repos/AlexanderSeel/TheReFmB1rth/releases/tags';
const CHANNEL_ROOT='https://raw.githubusercontent.com/AlexanderSeel/TheReFmB1rth/firmware-channel';
const state = {
  access:null, updater:null, pkg:null, editorOut:null, machine:0, note:null,
  pattern:0, selectedStep:0, selectedVoice:0,
  acidPatterns:Array.from({length:2},()=>Array.from({length:8},()=>Array.from({length:16},()=>({note:48,type:1,prob:100,accent:false,slide:false})))),
  drumPatterns:Array.from({length:2},()=>Array.from({length:8},()=>Array.from({length:16},()=>Array(11).fill(0))))
};

function log(msg){const el=$('#installLog');el.textContent += `\n${new Date().toLocaleTimeString()}  ${msg}`;el.scrollTop=el.scrollHeight;}
function setMidiStatus(text,on=false){const e=$('#midiStatus');e.textContent=text;e.classList.toggle('online',on);}
function updateUnlock(){const ok=!!state.pkg&&!!state.access&&$('#riskCheck').checked&&$('#confirmText').value.trim()==='FLASH THE FM-1';$('#installBtn').disabled=!ok;$('#resumeBtn').disabled=!ok;}
function fmtBytes(n){return n>1024*1024?`${(n/1024/1024).toFixed(2)} MB`:`${(n/1024).toFixed(1)} KB`;}
async function sha256Hex(bytes){const digest=await crypto.subtle.digest('SHA-256',bytes);return [...new Uint8Array(digest)].map(x=>x.toString(16).padStart(2,'0')).join('');}

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
    const now=Date.now();
    const mr=await fetch(`${CHANNEL_ROOT}/latest.json?v=${now}`,{cache:'no-store'});
    if(!mr.ok)throw new Error(`Browser firmware channel is not available yet (${mr.status}). Publish a new hardware-test release or choose the .fwsc locally.`);
    const meta=await mr.json();
    if(!/^v.+-beta(?:\.|-)?\d+$/i.test(meta.tag||''))throw new Error(`Browser channel has an invalid beta tag: ${meta.tag||'unknown'}.`);
    if(!/\.fwsc$/i.test(meta.asset||''))throw new Error(`Browser channel asset is not an .fwsc package: ${meta.asset||'unknown'}.`);
    if(!/^[0-9a-f]{64}$/i.test(meta.sha256||''))throw new Error('Browser channel metadata has an invalid SHA-256.');

    log(`Browser channel points to ${meta.tag}; verifying that exact GitHub release…`);
    const releaseUrl=`${RELEASE_BY_TAG_API}/${encodeURIComponent(meta.tag)}?v=${now}`;
    const rs=await fetch(releaseUrl,{cache:'no-store',headers:{Accept:'application/vnd.github+json'}});
    if(!rs.ok)throw new Error(`GitHub release ${meta.tag} is not available (${rs.status}). Choose the .fwsc locally if GitHub is still updating.`);
    const release=await rs.json();
    if(release.draft)throw new Error(`GitHub release ${meta.tag} is still a draft.`);
    if(release.tag_name!==meta.tag)throw new Error(`GitHub tag mismatch: channel ${meta.tag}, release ${release.tag_name||'unknown'}.`);
    const asset=release.assets?.find(a=>a.name===meta.asset&&/\.fwsc$/i.test(a.name));
    if(!asset)throw new Error(`GitHub release ${meta.tag} does not contain channel asset ${meta.asset}.`);

    log(`Verified ${meta.tag} and ${meta.asset}; downloading browser-safe firmware mirror…`);
    const fw=await fetch(`${CHANNEL_ROOT}/${encodeURIComponent(meta.asset)}?v=${encodeURIComponent(meta.tag)}-${now}`,{cache:'no-store'});
    if(!fw.ok)throw new Error(`Firmware channel download ${fw.status}`);
    const bytes=new Uint8Array(await fw.arrayBuffer());
    const actual=await sha256Hex(bytes);
    if(actual.toLowerCase()!==meta.sha256.toLowerCase())throw new Error(`Firmware mirror SHA-256 mismatch (${actual}).`);
    log(`Mirror verified: SHA-256 ${actual}`);
    await loadPackage(bytes,`GitHub ${meta.tag}`);
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

function currentAcidStep(){return state.acidPatterns[state.machine][state.pattern][state.selectedStep];}
function currentDrumState(){return state.drumPatterns[state.machine-2][state.pattern][state.selectedStep][state.selectedVoice];}
function pushAcidStep(){
  const s=currentAcidStep();cc(102,state.selectedStep);cc(103,s.note);cc(104,s.type);cc(105,(s.accent?1:0)|(s.slide?2:0));cc(106,s.prob);
}
function pushDrumStep(){cc(102,state.selectedStep);cc(104,state.selectedVoice);cc(105,currentDrumState());}
function refreshSequencer(){
  $('#seqSubtitle').textContent=`${machineNames[state.machine]} · pattern ${String.fromCharCode(65+state.pattern)}`;
  $('#seqMode').textContent=`STEP ${String(state.selectedStep+1).padStart(2,'0')}`;
  $$('#seqSteps button').forEach((b,i)=>b.classList.toggle('selected',i===state.selectedStep));
  const acid=state.machine<2;
  $('#acidStepEditor').classList.toggle('hidden',!acid);$('#drumStepEditor').classList.toggle('hidden',acid);
  if(acid){
    const s=currentAcidStep();
    $$('#stepTypes button').forEach(b=>b.classList.toggle('active',Number(b.dataset.stepType)===s.type));
    $('#stepNote').value=s.note;$('#stepProbability').value=s.prob;$('#stepProbability').nextElementSibling.textContent=`${s.prob}%`;
    $('#stepAccent').classList.toggle('active',s.accent);$('#stepSlide').classList.toggle('active',s.slide);
    $$('#seqSteps button').forEach((b,i)=>{const x=state.acidPatterns[state.machine][state.pattern][i];b.dataset.state=x.type===0?'rest':x.type===2?'tie':'note';b.textContent=x.type===0?'—':x.type===2?'T':String(i+1);});
  }else{
    const bank=state.drumPatterns[state.machine-2][state.pattern];
    $$('#drumStates button').forEach(b=>b.classList.toggle('active',Number(b.dataset.drumState)===currentDrumState()));
    $$('#voiceSelect button').forEach((b,i)=>b.classList.toggle('active',i===state.selectedVoice));
    $$('#seqSteps button').forEach((b,i)=>{const v=bank[i][state.selectedVoice];b.dataset.state=v===0?'rest':v===2?'accent':'hit';b.textContent=String(i+1);});
  }
}

$$('.machine').forEach(b=>b.addEventListener('click',()=>{state.machine=Number(b.dataset.machine);$$('.machine').forEach(x=>x.classList.toggle('active',x===b));const acid=state.machine<2;$('#acidPanel').classList.toggle('hidden',!acid);$('#drumPanel').classList.toggle('hidden',acid);$('#acidName').textContent=machineNames[state.machine];$('#drumName').textContent=machineNames[state.machine];$('#mixTrackName').textContent=machineNames[state.machine];refreshSequencer();}));
$$('input[data-cc]').forEach(r=>r.addEventListener('input',()=>{r.nextElementSibling.textContent=r.value;cc(Number(r.dataset.cc),r.value);}));
$$('[data-toggle-cc]').forEach(b=>b.addEventListener('click',()=>{b.classList.toggle('active');cc(Number(b.dataset.toggleCc),b.classList.contains('active')?127:0);}));
$$('[data-wave]').forEach(b=>b.addEventListener('click',()=>{$$('[data-wave]').forEach(x=>x.classList.remove('active'));b.classList.add('active');cc(77,Number(b.dataset.wave)?127:0);}));

const notes=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];const keyboard=$('#keyboard');
for(let midi=36;midi<=59;midi++){const b=document.createElement('button');b.className='key '+(notes[midi%12].includes('#')?'black':'');b.textContent=`${notes[midi%12]}${Math.floor(midi/12)-1}`;const on=()=>{if(state.machine>1)return;state.note=midi;send([0x90+channels[state.machine],midi,100]);};const off=()=>{if(state.note===midi){send([0x80+channels[state.machine],midi,0]);state.note=null;}};b.addEventListener('pointerdown',on);b.addEventListener('pointerup',off);b.addEventListener('pointerleave',off);keyboard.appendChild(b);}

const patterns=$('#patterns');for(let i=0;i<8;i++){const b=document.createElement('button');b.textContent=String.fromCharCode(65+i);b.addEventListener('click',()=>{state.pattern=i;$$('#patterns button').forEach(x=>x.classList.remove('active'));b.classList.add('active');program(i);refreshSequencer();});patterns.appendChild(b);}patterns.firstElementChild.classList.add('active');
const drumPads=$('#drumPads');for(const [name,note] of drumMap){const b=document.createElement('button');b.textContent=name;b.addEventListener('pointerdown',()=>send([0x90+channels[state.machine],note,110]));b.addEventListener('pointerup',()=>send([0x80+channels[state.machine],note,0]));drumPads.appendChild(b);}

const seqSteps=$('#seqSteps');for(let i=0;i<16;i++){const b=document.createElement('button');b.textContent=String(i+1);b.addEventListener('click',()=>{state.selectedStep=i;cc(102,i);refreshSequencer();});seqSteps.appendChild(b);}
$$('#stepTypes button').forEach(b=>b.addEventListener('click',()=>{const s=currentAcidStep();s.type=Number(b.dataset.stepType);cc(102,state.selectedStep);cc(104,s.type);refreshSequencer();}));
$('#stepNote').addEventListener('change',()=>{const s=currentAcidStep();s.note=Math.max(0,Math.min(127,Number($('#stepNote').value)||0));cc(102,state.selectedStep);cc(103,s.note);refreshSequencer();});
$('#stepProbability').addEventListener('input',()=>{const s=currentAcidStep();s.prob=Number($('#stepProbability').value);$('#stepProbability').nextElementSibling.textContent=`${s.prob}%`;cc(102,state.selectedStep);cc(106,s.prob);});
$('#stepAccent').addEventListener('click',()=>{const s=currentAcidStep();s.accent=!s.accent;pushAcidStep();refreshSequencer();});
$('#stepSlide').addEventListener('click',()=>{const s=currentAcidStep();s.slide=!s.slide;pushAcidStep();refreshSequencer();});
const voiceSelect=$('#voiceSelect');drumMap.forEach(([name],i)=>{const b=document.createElement('button');b.textContent=name;b.addEventListener('click',()=>{state.selectedVoice=i;cc(104,i);refreshSequencer();});voiceSelect.appendChild(b);});
$$('#drumStates button').forEach(b=>b.addEventListener('click',()=>{state.drumPatterns[state.machine-2][state.pattern][state.selectedStep][state.selectedVoice]=Number(b.dataset.drumState);pushDrumStep();refreshSequencer();}));

installReadback({state,refreshSequencer,ensureMidi,setMidiStatus});
updateUnlock();refreshSequencer();