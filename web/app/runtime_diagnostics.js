// SPDX-License-Identifier: GPL-3.0-only
const COMMAND=91;
const HDR=[0xF0,0x7D,0x46,0x4C,COMMAND];
const DEVICE_RE=/fm-1|felucca|composite|sinco|usb-midi/i;
const HALF_FRAMES=128;
const FS=44100;
const MAX_SUSTAINED=75;
const MAX_PEAK=82;
const MIN_USTACK_FREE=4096;
const MIN_SSTACK_FREE=1024;
const FIELDS=['magic','halves','last_us','max_us','cpu_q8','late','nested','timer_irqs','ui_frames','ustack_used','ustack_free','sstack_used','sstack_free'];

function r32(a,o){return (a[o]|a[o+1]<<7|a[o+2]<<14|a[o+3]<<21|(a[o+4]&15)<<28)>>>0;}
function status(text,on=true){const e=document.querySelector('#midiStatus');if(!e)return;e.textContent=text;e.classList.toggle('online',on);}
async function pair(){
  if(!navigator.requestMIDIAccess)throw new Error('Web MIDI is unavailable. Use current Chrome or Edge on HTTPS/localhost.');
  const access=await navigator.requestMIDIAccess({sysex:true});
  const inputs=[...access.inputs.values()].filter(x=>x.state!=='disconnected'&&DEVICE_RE.test(x.name||''));
  const outputs=[...access.outputs.values()].filter(x=>x.state!=='disconnected'&&DEVICE_RE.test(x.name||''));
  if(inputs.length!==1||outputs.length!==1)throw new Error(`Need exactly one FM-1 MIDI input/output (found ${inputs.length}/${outputs.length}).`);
  await inputs[0].open();await outputs[0].open();return {input:inputs[0],output:outputs[0]};
}
function request(link,timeout=1800){return new Promise((resolve,reject)=>{let timer;const done=(fn,v)=>{clearTimeout(timer);link.input.removeEventListener('midimessage',onMidi);fn(v);};const onMidi=e=>{const d=[...e.data];if(d.length<7||d[0]!==0xF0||d[d.length-1]!==0xF7||d[1]!==0x7D||d[2]!==0x46||d[3]!==0x4C||d[4]!==COMMAND)return;done(resolve,d.slice(5,-1));};link.input.addEventListener('midimessage',onMidi);timer=setTimeout(()=>done(reject,new Error('FM-1 diagnostic capture timed out.')),timeout);try{link.output.send([...HDR,0xF7]);}catch(e){done(reject,e);}});}
function evaluate(snapshot){
  const deadline=HALF_FRAMES*1000000/FS;
  const sustained=snapshot.cpu_q8*100/256;
  const last=snapshot.last_us*100/deadline;
  const peak=snapshot.max_us*100/deadline;
  const failures=[];
  if(snapshot.magic!==0x44424731)failures.push(`diagnostic magic mismatch: 0x${snapshot.magic.toString(16)}`);
  if(snapshot.halves<=0)failures.push('no rendered audio halves were captured');
  if(snapshot.late!==0)failures.push(`DMA deadline misses detected: late=${snapshot.late}`);
  if(sustained>MAX_SUSTAINED)failures.push(`sustained CPU ${sustained.toFixed(1)}% exceeds ${MAX_SUSTAINED}% gate`);
  if(peak>MAX_PEAK)failures.push(`peak render ${peak.toFixed(1)}% exceeds ${MAX_PEAK}% gate`);
  if(snapshot.ustack_free<MIN_USTACK_FREE)failures.push(`user-stack free ${snapshot.ustack_free} B is below ${MIN_USTACK_FREE} B gate`);
  if(snapshot.sstack_free<MIN_SSTACK_FREE)failures.push(`supervisor-stack free ${snapshot.sstack_free} B is below ${MIN_SSTACK_FREE} B gate`);
  return {pass:failures.length===0,half_frames:HALF_FRAMES,deadline_us:Number(deadline.toFixed(3)),halves:snapshot.halves,late:snapshot.late,last_us:snapshot.last_us,max_us:snapshot.max_us,sustained_cpu_pct:Number(sustained.toFixed(3)),last_render_pct:Number(last.toFixed(3)),peak_render_pct:Number(peak.toFixed(3)),stack:{user:{used_bytes:snapshot.ustack_used,free_bytes:snapshot.ustack_free},supervisor:{used_bytes:snapshot.sstack_used,free_bytes:snapshot.sstack_free},limits:{min_user_free_bytes:MIN_USTACK_FREE,min_supervisor_free_bytes:MIN_SSTACK_FREE}},limits:{max_sustained_pct:MAX_SUSTAINED,max_peak_pct:MAX_PEAK},failures};
}
function render(panel,snapshot,report){
  panel.innerHTML='';
  const title=document.createElement('strong');title.textContent=report.pass?'RUNTIME GATE · PASS':'RUNTIME GATE · FAIL';panel.appendChild(title);
  const pre=document.createElement('pre');pre.className='log';pre.textContent=[`CPU sustained  ${report.sustained_cpu_pct.toFixed(1)}%  (≤ ${MAX_SUSTAINED}%)`,`Render last    ${report.last_render_pct.toFixed(1)}%`,`Render peak    ${report.peak_render_pct.toFixed(1)}%  (≤ ${MAX_PEAK}%)`,`DMA late       ${snapshot.late}  (${snapshot.late===0?'OK':'FAIL'})`,`User stack     ${snapshot.ustack_free} B free  (≥ ${MIN_USTACK_FREE})`,`Supervisor     ${snapshot.sstack_free} B free  (≥ ${MIN_SSTACK_FREE})`,`Audio halves   ${snapshot.halves}`,...(report.failures.length?['',...report.failures.map(x=>`FAIL: ${x}`)]:[])].join('\n');panel.appendChild(pre);
  const dl=document.createElement('button');dl.className='secondary';dl.textContent='DOWNLOAD DIAGNOSTIC JSON';dl.addEventListener('click',()=>{const blob=new Blob([JSON.stringify(snapshot,null,2)+'\n'],{type:'application/json'});const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=`fm1-runtime-${new Date().toISOString().replace(/[:.]/g,'-')}.json`;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);});panel.appendChild(dl);
}
async function capture(panel){
  const link=await pair();status('CAPTURING RUNTIME…');const payload=await request(link);
  if(payload[0]!==1)throw new Error('FM-1 returned an unsupported diagnostic protocol version.');
  if(payload.length!==1+FIELDS.length*5)throw new Error(`Unexpected diagnostic payload length ${payload.length}.`);
  const snapshot={};FIELDS.forEach((name,i)=>snapshot[name]=r32(payload,1+i*5));
  const report=evaluate(snapshot);render(panel,snapshot,report);status(report.pass?'RUNTIME PASS':'RUNTIME FAIL',report.pass);
  window.dispatchEvent(new CustomEvent('refm-runtime-diagnostic',{detail:{captured_at:new Date().toISOString(),snapshot,report}}));
}
function install(){
  const connect=document.querySelector('#connectEditor');if(!connect)return;
  const btn=document.createElement('button');btn.id='captureRuntime';btn.className='secondary';btn.textContent='CAPTURE RUNTIME';connect.insertAdjacentElement('afterend',btn);
  const panel=document.createElement('div');panel.id='runtimeDiagnostics';panel.className='card';panel.hidden=true;connect.closest('.hero')?.insertAdjacentElement('afterend',panel);
  btn.addEventListener('click',async()=>{btn.disabled=true;panel.hidden=false;try{await capture(panel);}catch(e){status('RUNTIME CAPTURE FAILED',false);panel.textContent=e.message;}finally{btn.disabled=false;}});
}
install();
