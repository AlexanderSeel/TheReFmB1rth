// SPDX-License-Identifier: GPL-3.0-only
const CHANNEL_META='https://raw.githubusercontent.com/AlexanderSeel/TheReFmB1rth/firmware-channel/latest.json';
const STORAGE_KEY='refm-beta-physical-validation-v1';
const SCHEMA=1;
const CHECKS=[
  ['install','Install & boot','beta firmware installs through loader-mediated flow and boots normally'],
  ['home','LCD / UI','HOME renders correctly on the 240×240 LCD'],
  ['acid','LCD / UI','ACID renders correctly and labels match physical controls'],
  ['step','LCD / UI','STEP renders correctly and 16-step selection is usable'],
  ['drum','LCD / UI','DRUM renders correctly and lane/state editing is usable'],
  ['mixer','LCD / UI','MIXER renders correctly and remains responsive while playing'],
  ['fx','LCD / UI','FX renders correctly and remains responsive while playing'],
  ['song','LCD / UI','SONG renders correctly'],
  ['project','LCD / UI','PROJECT renders correctly with four project slots'],
  ['encoders','Physical controls','K1–K4 / PRESET / ALGO / SELECT match the LCD labels'],
  ['keys','Physical controls','keyboard plays/releases ACID A and B without stuck notes'],
  ['transport','Physical controls','physical PLAY/STOP drives ReFm transport and Pattern A is audible'],
  ['step_edit','Sequencer','ACID NOTE / REST / TIE / Accent / Slide / probability editing works'],
  ['drum_edit','Sequencer','808/909 OFF / HIT / ACCENT editing works'],
  ['browser_sync','Browser','device → browser state synchronization matches the FM-1'],
  ['project_backup','Browser','full project backup downloads successfully'],
  ['project_restore','Browser','project restore reconstructs the live project correctly'],
  ['usb_midi','I/O','USB MIDI note/control/transport verified'],
  ['trs_midi','I/O','TRS MIDI note/control/clock verified'],
  ['usb_audio','I/O','USB audio verified without dropouts'],
  ['physical_save','Persistence','physical PROJECT SAVE/LOAD survives reboot'],
  ['corrupt_recovery','Persistence','invalid/corrupted project recovery is safe'],
  ['rollback','Recovery','return to official/Felucca firmware proven'],
  ['repeat_cycle','Recovery','update → beta → rollback cycle repeated successfully']
];

function emptyState(){
  return {schema:SCHEMA,target:null,started_at:new Date().toISOString(),updated_at:new Date().toISOString(),environment:{user_agent:navigator.userAgent},checks:Object.fromEntries(CHECKS.map(([id])=>[id,{status:'pending',note:''}])),runtime:{factory:null,stress:null},notes:''};
}
function normalize(raw){
  const s=raw&&raw.schema===SCHEMA?raw:emptyState();
  s.checks=s.checks||{};for(const [id] of CHECKS){if(!s.checks[id])s.checks[id]={status:'pending',note:''};}
  s.runtime=s.runtime||{factory:null,stress:null};s.environment=s.environment||{user_agent:navigator.userAgent};return s;
}
function load(){try{return normalize(JSON.parse(localStorage.getItem(STORAGE_KEY)||'null'));}catch{return emptyState();}}
let state=load();let lastRuntime=null;
function save(){state.updated_at=new Date().toISOString();localStorage.setItem(STORAGE_KEY,JSON.stringify(state));renderSummary();}
function esc(s=''){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
function download(name,text,type='application/json'){const a=document.createElement('a');const u=URL.createObjectURL(new Blob([text],{type}));a.href=u;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(u),1000);}
function runtimeLabel(x){if(!x)return 'NOT CAPTURED';return x.report?.pass?`PASS · ${x.report.sustained_cpu_pct.toFixed(1)}% CPU · peak ${x.report.peak_render_pct.toFixed(1)}% · late ${x.snapshot.late}`:`FAIL · ${(x.report?.failures||[]).join('; ')}`;}
function counts(){const vals=CHECKS.map(([id])=>state.checks[id]?.status||'pending');return {pass:vals.filter(x=>x==='pass').length,fail:vals.filter(x=>x==='fail').length,pending:vals.filter(x=>x==='pending').length,na:vals.filter(x=>x==='na').length,total:vals.length};}
function allReady(){return CHECKS.every(([id])=>state.checks[id]?.status==='pass')&&state.runtime.factory?.report?.pass===true&&state.runtime.stress?.report?.pass===true;}
function renderSummary(){
  const c=counts(),el=document.querySelector('#validationSummary');if(!el)return;
  const target=state.target?`${state.target.tag} · ${String(state.target.commit||'').slice(0,10)}`:'target release not loaded';
  el.innerHTML=`<strong>${allReady()?'HARDWARE GATES · PASS':'HARDWARE GATES · OPEN'}</strong><span>${esc(target)}</span><span>${c.pass}/${c.total} checks passed · ${c.fail} failed · ${c.pending} pending · ${c.na} N/A</span><span>Factory runtime: ${esc(runtimeLabel(state.runtime.factory))}</span><span>Stress runtime: ${esc(runtimeLabel(state.runtime.stress))}</span>`;
  el.classList.toggle('validationPass',allReady());
}
function renderRows(){
  const body=document.querySelector('#validationChecks');if(!body)return;body.innerHTML='';let group='';
  for(const [id,g,label] of CHECKS){
    if(g!==group){group=g;const h=document.createElement('div');h.className='validationGroup';h.textContent=g;body.appendChild(h);}
    const row=document.createElement('div');row.className='validationRow';row.dataset.id=id;
    row.innerHTML=`<div class="validationLabel">${esc(label)}</div><select aria-label="${esc(label)} status"><option value="pending">NOT TESTED</option><option value="pass">PASS</option><option value="fail">FAIL</option><option value="na">N/A</option></select><input aria-label="${esc(label)} notes" placeholder="notes / observation" value="${esc(state.checks[id].note||'')}">`;
    const sel=row.querySelector('select'),note=row.querySelector('input');sel.value=state.checks[id].status||'pending';row.dataset.status=sel.value;
    sel.addEventListener('change',()=>{state.checks[id].status=sel.value;row.dataset.status=sel.value;save();});
    note.addEventListener('change',()=>{state.checks[id].note=note.value.trim();save();});body.appendChild(row);
  }
}
async function loadTarget(){
  const status=document.querySelector('#validationTarget');status.textContent='Loading firmware-channel metadata…';
  try{
    const r=await fetch(`${CHANNEL_META}?v=${Date.now()}`,{cache:'no-store'});if(!r.ok)throw new Error(`HTTP ${r.status}`);const m=await r.json();if(!m.tag||!m.commit||!m.sha256)throw new Error('metadata incomplete');
    if(state.target?.commit&&state.target.commit!==m.commit){const previous=state.target.commit;state=emptyState();state.notes=`Validation reset automatically because target changed from ${previous} to ${m.commit}.`;renderRows();renderRuntime();const notes=document.querySelector('#validationNotes');if(notes)notes.value=state.notes;}
    state.target={tag:m.tag,commit:m.commit,asset:m.asset,sha256:m.sha256,bytes:m.bytes,captured_at:new Date().toISOString()};status.textContent=`${m.tag} · ${m.commit.slice(0,12)} · ${m.sha256.slice(0,12)}…`;save();
  }catch(e){status.textContent=`Target metadata unavailable: ${e.message}`;}
}
function assignRuntime(slot){if(!lastRuntime)return;state.runtime[slot]={...lastRuntime,assigned_at:new Date().toISOString()};save();renderRuntime();}
function renderRuntime(){
  const last=document.querySelector('#validationRuntimeLast');if(last)last.textContent=lastRuntime?runtimeLabel(lastRuntime):'Use CAPTURE RUNTIME on the Live Editor, then assign the result here.';
  const f=document.querySelector('#validationFactory');if(f)f.textContent=runtimeLabel(state.runtime.factory);
  const s=document.querySelector('#validationStress');if(s)s.textContent=runtimeLabel(state.runtime.stress);
}
function reportObject(){return {...state,summary:{...counts(),ready:allReady()},exported_at:new Date().toISOString()};}
function markdown(){
  const r=reportObject(),lines=[`# TheReFmB1rth physical validation`, '',`- Target: ${r.target?`${r.target.tag} (${r.target.commit})`:'unknown'}`,`- Firmware SHA-256: ${r.target?.sha256||'unknown'}`,`- Started: ${r.started_at}`,`- Updated: ${r.updated_at}`,`- Overall: ${r.summary.ready?'PASS':'OPEN / FAIL'}`,'','## Hardware checks',''];
  let g='';for(const [id,group,label] of CHECKS){if(group!==g){g=group;lines.push(`### ${group}`);}const x=r.checks[id];lines.push(`- [${x.status==='pass'?'x':' '}] **${x.status.toUpperCase()}** — ${label}${x.note?` — ${x.note}`:''}`);}lines.push('','## Runtime gates','',`- Factory Pattern A: ${runtimeLabel(r.runtime.factory)}`,`- Worst-case stress: ${runtimeLabel(r.runtime.stress)}`,'','## Notes','',r.notes||'—','');return lines.join('\n');
}
function install(){
  renderRows();renderSummary();renderRuntime();
  document.querySelector('#validationRefreshTarget')?.addEventListener('click',loadTarget);
  document.querySelector('#validationAssignFactory')?.addEventListener('click',()=>assignRuntime('factory'));
  document.querySelector('#validationAssignStress')?.addEventListener('click',()=>assignRuntime('stress'));
  const notes=document.querySelector('#validationNotes');if(notes){notes.value=state.notes||'';notes.addEventListener('change',()=>{state.notes=notes.value.trim();save();});}
  document.querySelector('#validationExportJson')?.addEventListener('click',()=>download(`refm-physical-validation-${Date.now()}.json`,JSON.stringify(reportObject(),null,2)+'\n'));
  document.querySelector('#validationExportMd')?.addEventListener('click',()=>download(`refm-physical-validation-${Date.now()}.md`,markdown()+'\n','text/markdown'));
  document.querySelector('#validationReset')?.addEventListener('click',()=>{if(!confirm('Reset all locally stored physical validation results?'))return;state=emptyState();localStorage.removeItem(STORAGE_KEY);renderRows();renderSummary();renderRuntime();if(notes)notes.value='';loadTarget();});
  window.addEventListener('refm-runtime-diagnostic',e=>{lastRuntime=e.detail;renderRuntime();});
  if(state.target){document.querySelector('#validationTarget').textContent=`${state.target.tag} · ${state.target.commit.slice(0,12)} · ${state.target.sha256.slice(0,12)}…`;}
  else loadTarget();
}
install();
