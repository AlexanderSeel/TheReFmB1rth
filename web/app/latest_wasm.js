// SPDX-License-Identifier: GPL-3.0-only
const PAGES_BASE='https://alexanderseel.github.io/TheReFmB1rth/';
const localChannel=new URL('../wasm-channel.json',import.meta.url);
const remoteChannel=new URL('wasm-channel.json',PAGES_BASE);
const localWasm=new URL('../refm.wasm',import.meta.url);
const remoteWasm=new URL('refm.wasm',PAGES_BASE);

async function channel(){
  for(const u of [localChannel,remoteChannel]){
    try{const r=await fetch(u,{cache:'no-store'});if(r.ok)return await r.json();}catch{}
  }
  return null;
}
async function download(url,name){
  try{
    const r=await fetch(url,{cache:'no-store'});if(!r.ok)throw new Error(`HTTP ${r.status}`);
    const blob=await r.blob(),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=name;document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(a.href),1000);
  }catch{window.open(url.href,'_blank','noopener');}
}
export async function installLatestWasmControl(){
  if(document.querySelector('#virtualDownloadWasm'))return;
  const host=document.querySelector('#virtual .heroActions');if(!host)return;
  const btn=document.createElement('button');btn.id='virtualDownloadWasm';btn.className='secondary';btn.textContent='DOWNLOAD LATEST WASM';
  const meta=document.createElement('span');meta.id='virtualWasmMeta';meta.className='status';meta.textContent='WASM latest';
  host.append(btn,meta);
  const info=await channel();
  if(info?.commit)meta.textContent=`WASM ${String(info.commit).slice(0,8)}${info.sha256?' · verified':''}`;
  btn.addEventListener('click',async()=>{
    btn.disabled=true;const old=btn.textContent;btn.textContent='DOWNLOADING…';
    try{
      let url=remoteWasm;
      try{const r=await fetch(localWasm,{method:'HEAD',cache:'no-store'});if(r.ok)url=localWasm;}catch{}
      await download(url,`TheReFmB1rth-${info?.commit?String(info.commit).slice(0,8):'latest'}.wasm`);
    }finally{btn.disabled=false;btn.textContent=old;}
  });
}
installLatestWasmControl();
