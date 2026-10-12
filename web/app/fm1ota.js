// SPDX-License-Identifier: GPL-3.0-only
// FM-1 WebMIDI updater, adapted from pinned Felucca 1.0.5 browser updater.
const HS_QUERY = [0xF0,0x00,0x32,0x45,0x00,0x00,0x00,0x40,0x7F,0xF7];
const UPGRADE = [0xF0,0x22,0x24,0x35,0x7F,0xF7];
const MAXDATA = 512;
const sleep = ms => new Promise(r => setTimeout(r, ms));
const isOta = id => /^ota-/i.test(id.model);

function fail(code, message, detail='') { const e = new Error(message); e.code=code; e.detail=detail; return e; }
export function pack7(data){ const out=[]; let acc=0,nb=0; for(const b of data){acc|=b<<nb;nb+=8;while(nb>=7){out.push(acc&0x7f);acc>>>=7;nb-=7;}} if(nb)out.push(acc&0x7f); return out; }
export function unpack7(data){ const out=[]; let acc=0,nb=0; for(const b of data){acc|=(b&0x7f)<<nb;nb+=7;while(nb>=8){out.push(acc&0xff);acc>>>=8;nb-=8;}} return out; }

function response(addr,data,fl=0){ const len=data.length; const body=[0x00,0x59,0x30,(len+8)&255,((len+8)>>8)&255,0,fl,addr&255,(addr>>>8)&255,(addr>>>16)&255,(addr>>>24)&255,len&255,(len>>8)&255,0,...data]; let s=0; for(let i=6;i<body.length;i++)s+=body[i]; body.push(~s&255); return [0xf0,...pack7(body),0xf7]; }
function parseRequest(pkt){ if(pkt[0]!==0xf0||pkt[pkt.length-1]!==0xf7)return null; const u=unpack7(pkt.slice(1,-1)); if(u.length!==15||u[0]!==0||u[1]!==0x59||u[2]!==0x30)return null; let s=0; for(let i=6;i<14;i++)s+=u[i]; if((~s&255)!==u[14])return null; return {fl:u[6],addr:(u[7]|u[8]<<8|u[9]<<16|u[10]<<24)>>>0,len:u[11]|u[12]<<8|u[13]<<16}; }
function parseIdentity(pkt){ const d=unpack7(pkt.slice(1,-1)); if(d.length!==34||d[0]!==0||d[1]!==0x59||d[2]!==0x11)return null; const txt=String.fromCharCode(...d.slice(6,33)).replace(/\0+$/,''); const m=/^([^_]+)_(\d+)$/.exec(txt); return m?{text:txt,model:m[1],version:parseInt(m[2],10)}:null; }

class Link{
  constructor(input,output){this.input=input;this.output=output;this.q=[];this.waiter=null;input.onmidimessage=ev=>{if(ev.data[0]!==0xf0)return;this.q.push(Array.from(ev.data));if(this.waiter){const w=this.waiter;this.waiter=null;w();}};}
  get lost(){return this.input.state==='disconnected'||this.output.state==='disconnected';}
  send(bytes){try{this.output.send(bytes);return true;}catch{return false;}}
  drain(){this.q.length=0;}
  async read(ms){if(!this.q.length){let tm;await Promise.race([new Promise(r=>{this.waiter=r;}),new Promise(r=>{tm=setTimeout(r,ms);})]);clearTimeout(tm);this.waiter=null;}return this.q.shift()||null;}
  close(){this.input.onmidimessage=null;}
}

async function handshake(link,tries=3){link.drain();for(let t=0;t<tries;t++){if(!link.send(HS_QUERY))return null;const end=Date.now()+1000;while(Date.now()<end){const p=await link.read(end-Date.now());if(!p)break;const id=parseIdentity(p);if(id)return id;}}return null;}

export class Updater{
  constructor(access,log=()=>{}){this.access=access;this.log=log;}
  candidatePairs(){
    const outs=[...this.access.outputs.values()].filter(o=>o.state!=='disconnected');
    const pairs=[];
    for(const input of this.access.inputs.values()){
      if(input.state==='disconnected')continue;
      if(!/fm-1|felucca|ota|composite|sinco|usb-midi/i.test(input.name||''))continue;
      const output=outs.find(o=>o.name===input.name);
      if(output)pairs.push({input,output});
    }
    return pairs;
  }
  async find(filter){
    const pairs=this.candidatePairs();
    if(pairs.length>1)throw fail('multiple',`More than one possible FM-1 MIDI device is connected (${pairs.map(p=>p.input.name).join(', ')}). Disconnect the others before flashing.`);
    for(const {input,output} of pairs){
      try{await input.open();await output.open();}catch{continue;}
      const link=new Link(input,output);const id=await handshake(link,2);
      if(id&&(!filter||filter(id)))return{link,id,name:input.name};
      link.close();
    }
    return null;
  }
  async identify(){const dev=await this.find(id=>!isOta(id));if(!dev)return null;const info={name:dev.name,...dev.id};dev.link.close();return info;}
  async waitFor(filter,ms){const end=Date.now()+ms;while(Date.now()<end){const dev=await this.find(filter);if(dev)return dev;await sleep(1000);}return null;}
  async serve(link,image,finishAddr,idleMs,onProgress){let served=0,last=Date.now();for(;;){const pkt=await link.read(1000);if(!pkt){if(link.lost)return{served,finished:false,lost:true};if(Date.now()-last>idleMs)return{served,finished:false};continue;}const r=parseRequest(pkt);if(!r)continue;last=Date.now();if(r.addr===0xe0000000||r.addr===0xf0000000){link.send(response(r.addr,[...new TextEncoder().encode('success'),0]));if(r.addr===finishAddr)return{served,finished:true};continue;}if(r.len>MAXDATA||r.addr+r.len>image.length)throw fail('badreq',`Bad device request ${r.addr.toString(16)}+${r.len}`);await sleep(10);if(!link.send(response(r.addr,Array.from(image.subarray(r.addr,r.addr+r.len)),r.fl)))return{served,finished:false,lost:true};served++;onProgress?.(served,r.addr);}}
  async write(ota,image,step){ota.link.send(UPGRADE);await sleep(2000);const result=await this.serve(ota.link,image,0xf0000000,180000,n=>step('write',n));ota.link.close();return result;}
  async install(image,product,onStep=()=>{}){const dev=await this.find(id=>!isOta(id));if(!dev)throw fail('notfound','FM-1 not found. Check USB and close other MIDI applications.');const model=product.split('_')[0];if(dev.id.model!==model){dev.link.close();throw fail('model',`Connected device is ${dev.id.text}; package is ${product}.`);}onStep('start',dev.id.text);dev.link.send(UPGRADE);await sleep(2000);const s1=await this.serve(dev.link,image,0xe0000000,8000,n=>onStep('verify',n));dev.link.close();if(!s1.finished)throw fail(s1.lost?'lost':'stopped','The running firmware stopped responding before loader handoff.');onStep('loader');await sleep(3000);const ota=await this.waitFor(id=>id.model==='ota-'+model,30000);if(!ota)throw fail('noloader','Update loader did not appear. Replug USB; the device should remain in update mode.');const s2=await this.write(ota,image,onStep);if(!s2.finished)throw fail(s2.lost?'lost':'stopped','Loader stopped during write. Replug and retry to resume.');onStep('reboot');await sleep(3000);const back=await this.waitFor(id=>!isOta(id),40000);if(!back)throw fail('noreturn','Device did not return after flashing. Power-cycle it.');back.link.close();if(back.id.text!==product)throw fail('mismatch',`Device returned as ${back.id.text}, expected ${product}.`,back.id.text);onStep('done',back.id.text);return back.id.text;}
  async resume(image,product,onStep=()=>{}){const model=product.split('_')[0];const ota=await this.find(id=>id.model==='ota-'+model);if(!ota)return false;const result=await this.write(ota,image,onStep);if(!result.finished)return false;onStep('reboot');await sleep(3000);const back=await this.waitFor(id=>id.model===model,40000);if(!back)throw fail('noreturn','FM-1 did not return after resuming.');back.link.close();onStep('done',back.id.text);return true;}
}
