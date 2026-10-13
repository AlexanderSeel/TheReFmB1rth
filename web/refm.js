// SPDX-License-Identifier: GPL-3.0-only
// Local-development bridge used when web/app is served directly from the repo.
// The generated WASM runtime is published to the wasm-channel branch and served
// through jsDelivr with CORS-safe module/WASM MIME handling.
const CDN='https://cdn.jsdelivr.net/gh/AlexanderSeel/TheReFmB1rth@wasm-channel/';
export default async function createRefm(options={}){
  const bust=Date.now();
  const remote=await import(`${CDN}refm.js?v=${bust}`);
  const callerLocate=options.locateFile;
  return remote.default({...options,locateFile:p=>p.endsWith('.wasm')?`${CDN}refm.wasm?v=${bust}`:callerLocate?callerLocate(p):`${CDN}${p}?v=${bust}`});
}
