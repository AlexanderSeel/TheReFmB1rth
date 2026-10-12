// SPDX-License-Identifier: GPL-3.0-only
// Full ReFm project transfer over the pinned Felucca editor SysEx envelope.
const COMMAND=90;
const HDR=[0xF0,0x7D,0x46,0x4C,COMMAND];
const DEVICE_RE=/fm-1|felucca|composite|sinco|usb-midi/i;
const MAX_PROJECT=3840;
const CHUNK=256;

const sleep=ms=>new Promise(r=>setTimeout(r,ms));
function u32(v){v>>>=0;return [v&127,(v>>>7)&127,(v>>>14)&127,(v>>>21)&127,(v>>>28)&15];}
function r32(a,o=0){return (a[o]|a[o+1]<<7|a[o+2]<<14|a[o+3]<<21|(a[o+4]&15)<<28)>>>0;}
function pack7(bytes){const out=[];for(let i=0;i<bytes.length;i+=7){const n=Math.min(7,bytes.length-i);let mask=0;for(let j=0;j<n;j++)mask|=(bytes[i+j]>>7)<<j;out.push(mask);for(let j=0;j<n;j++)out.push(bytes[i+j]&127);}return out;}
function unpack7(a,expected){const out=[];for(let i=0;i<a.length&&out.length<expected;){const mask=a[i++];const n=Math.min(7,expected-out.length);if(mask>>n)throw new Error('Invalid project 7-bit group.');if(i+n>a.length)throw new Error('Truncated project 7-bit group.');for(let j=0;j<n;j++)out.push(a[i++]|((mask>>j)&1)<<7);}if(out.length!==expected)throw new Error(`Project chunk length mismatch (${out.length}/${expected}).`);return out;}
function crc32(bytes){let c=0xFFFFFFFF;for(const b of bytes){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xEDB88320:0);}return (~c)>>>0;}
function status(text,on=true){const e=document.querySelector('#midiStatus');if(!e)return;e.textContent=text;e.classList.toggle('online',on);}
function errText(rc){return rc===1?'malformed project transfer':rc===2?'CRC/project validation failed':rc===3?'stop playback before project transfer':rc===5?'transfer staging changed; retry':'device rejected project transfer';}

async function pair(){
  if(!navigator.requestMIDIAccess)throw new Error('Web MIDI is unavailable. Use current Chrome or Edge on HTTPS/localhost.');
  const access=await navigator.requestMIDIAccess({sysex:true});
  const inputs=[...access.inputs.values()].filter(x=>x.state!=='disconnected'&&DEVICE_RE.test(x.name||''));
  const outputs=[...access.outputs.values()].filter(x=>x.state!=='disconnected'&&DEVICE_RE.test(x.name||''));
  if(inputs.length!==1||outputs.length!==1)throw new Error(`Need exactly one FM-1 MIDI input/output (found ${inputs.length}/${outputs.length}).`);
  await inputs[0].open();await outputs[0].open();
  return {input:inputs[0],output:outputs[0]};
}

function request(link,args,timeout=2500){
  return new Promise((resolve,reject)=>{
    let timer;
    const done=(fn,v)=>{clearTimeout(timer);link.input.removeEventListener('midimessage',onMidi);fn(v);};
    const onMidi=e=>{
      const d=[...e.data];
      if(d.length<7||d[0]!==0xF0||d[d.length-1]!==0xF7||d[1]!==0x7D||d[2]!==0x46||d[3]!==0x4C||d[4]!==COMMAND)return;
      done(resolve,d.slice(5,-1));
    };
    link.input.addEventListener('midimessage',onMidi);
    timer=setTimeout(()=>done(reject,new Error('FM-1 project transfer timed out.')),timeout);
    try{link.output.send([...HDR,...args,0xF7]);}catch(e){done(reject,e);}
  });
}

async function capture(link){
  const r=await request(link,[0]);
  if(r[0]!==0||r.length<12)throw new Error('Invalid capture reply.');
  if(r[1])throw new Error(errText(r[1]));
  const len=r32(r,2),crc=r32(r,7);
  if(!len||len>MAX_PROJECT)throw new Error(`Device reported invalid project size ${len}.`);
  return {len,crc};
}

async function downloadProject(){
  const link=await pair();
  status('PROJECT BACKUP…');
  const {len,crc}=await capture(link);
  const data=new Uint8Array(len);
  for(let off=0;off<len;off+=CHUNK){
    const count=Math.min(CHUNK,len-off);
    const r=await request(link,[1,...u32(off),count&127,(count>>7)&127]);
    if(r[0]!==1||r.length<9)throw new Error('Invalid project chunk reply.');
    if(r[1])throw new Error(errText(r[1]));
    const gotOff=r32(r,2),gotCount=r[7]|r[8]<<7;
    if(gotOff!==off||gotCount!==count)throw new Error(`Unexpected project chunk ${gotOff}+${gotCount}.`);
    data.set(unpack7(r.slice(9),count),off);
    status(`PROJECT BACKUP ${Math.round((off+count)*100/len)}%`);
    await sleep(0);
  }
  const actual=crc32(data);
  if(actual!==crc)throw new Error(`Project transfer CRC mismatch (${actual.toString(16)} != ${crc.toString(16)}).`);
  const blob=new Blob([data],{type:'application/octet-stream'});
  const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=`TheReFmB1rth-project-${new Date().toISOString().replace(/[:.]/g,'-')}.refmproj`;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);
  status(`PROJECT BACKUP OK · ${len} B`);
}

async function restoreProject(file){
  const data=new Uint8Array(await file.arrayBuffer());
  if(!data.length||data.length>MAX_PROJECT)throw new Error(`Project file must be 1..${MAX_PROJECT} bytes.`);
  const link=await pair(),crc=crc32(data);
  status('PROJECT RESTORE…');
  let r=await request(link,[2,...u32(data.length),...u32(crc)]);
  if(r[0]!==2||r.length<2)throw new Error('Invalid restore-begin reply.');
  if(r[1])throw new Error(errText(r[1]));
  try{
    for(let off=0;off<data.length;off+=CHUNK){
      const chunk=data.subarray(off,Math.min(data.length,off+CHUNK));
      r=await request(link,[3,...u32(off),...pack7(chunk)]);
      if(r[0]!==3||r.length<7)throw new Error('Invalid restore-data reply.');
      if(r[1])throw new Error(errText(r[1]));
      const next=r32(r,2);
      if(next!==off+chunk.length)throw new Error(`Device restore offset mismatch (${next}/${off+chunk.length}).`);
      status(`PROJECT RESTORE ${Math.round(next*100/data.length)}%`);
      await sleep(8); // keep USB/editor queues comfortably below their bounded rings
    }
    r=await request(link,[4],4000);
    if(r[0]!==4||r.length<2)throw new Error('Invalid restore-commit reply.');
    if(r[1])throw new Error(errText(r[1]));
  }catch(e){try{await request(link,[5],800);}catch{}throw e;}
  status(`PROJECT RESTORE OK · ${data.length} B`);
  setTimeout(()=>document.querySelector('#syncEditor')?.click(),150);
}

function install(){
  const connect=document.querySelector('#connectEditor');if(!connect)return;
  const backup=document.createElement('button');backup.id='backupProject';backup.className='secondary';backup.textContent='BACKUP PROJECT';
  const restore=document.createElement('button');restore.id='restoreProject';restore.className='secondary';restore.textContent='RESTORE PROJECT';
  const input=document.createElement('input');input.type='file';input.accept='.refmproj,application/octet-stream';input.hidden=true;
  connect.insertAdjacentElement('afterend',restore);connect.insertAdjacentElement('afterend',backup);connect.parentElement.appendChild(input);
  backup.addEventListener('click',async()=>{backup.disabled=true;try{await downloadProject();}catch(e){status('PROJECT BACKUP FAILED',false);alert(e.message);}finally{backup.disabled=false;}});
  restore.addEventListener('click',()=>input.click());
  input.addEventListener('change',async()=>{const file=input.files?.[0];input.value='';if(!file)return;if(!confirm(`Restore ${file.name} to the live FM-1 project?\n\nPlayback must be stopped. The current unsaved live project will be replaced.`))return;restore.disabled=true;try{await restoreProject(file);}catch(e){status('PROJECT RESTORE FAILED',false);alert(e.message);}finally{restore.disabled=false;}});
}

install();
