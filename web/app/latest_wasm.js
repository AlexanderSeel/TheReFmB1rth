// SPDX-License-Identifier: GPL-3.0-only
const CDN='https://cdn.jsdelivr.net/gh/AlexanderSeel/TheReFmB1rth@wasm-channel/';
const localChannel=new URL('../wasm-channel.json',import.meta.url);
const localWasm=new URL('../refm.wasm',import.meta.url);
function remote(name){return new URL(`${name}?v=${Date.now()}`,CDN);}

async function channel(){
  for(const u of [localChannel,remote('wasm-channel.json')]){
    try{const r=await fetch(u,{cache:'no-store'});if(r.ok)return await r.json();}catch{}
  }
  return null;
}
async function download(url,name){
  const r=await fetch(url,{cache:'no-store'});if(!r.ok)throw new Error(`HTTP ${r.status}`);
  const blob=await r.blob(),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=name;document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(a.href),1000);
}
export async function installLatestWasmControl(){
  if(document.querySelector('#virtualDownloadWasm'))return;
  const host=document.querySelector('#virtual .heroActions');if(!host)return;
  const btn=document.createElement('button');btn.id='virtualDownloadWasm';btn.className='secondary';btn.textContent='DOWNLOAD LATEST WASM';
  const meta=document.createElement('span');meta.id='virtualWasmMeta';meta.className='status';meta.textContent='WASM channel';
  host.append(btn,meta);
  const info=await channel();
  if(info?.commit)meta.textContent=`WASM ${String(info.commit).slice(0,8)}${info.sha256?' · verified':''}`;
  btn.addEventListener('click',async()=>{
    btn.disabled=true;const old=btn.textContent;btn.textContent='DOWNLOADING…';
    try{
      let url=remote('refm.wasm');
      try{const r=await fetch(localWasm,{method:'HEAD',cache:'no-store'});if(r.ok)url=localWasm;}catch{}
      await download(url,`TheReFmB1rth-${info?.commit?String(info.commit).slice(0,8):'latest'}.wasm`);
    }catch(e){meta.textContent=`WASM download failed · ${e.message}`;}
    finally{btn.disabled=false;btn.textContent=old;}
  });
}
installLatestWasmControl();
