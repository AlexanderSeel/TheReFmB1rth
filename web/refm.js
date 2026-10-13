// SPDX-License-Identifier: GPL-3.0-only
// Local-development bridge used when web/app is served directly from the repo.
// Production Pages ships the generated build/wasm/refm.js at /refm.js instead.
import './app/latest_wasm.js';
const BASE='https://alexanderseel.github.io/TheReFmB1rth/';
export default async function createRefm(options={}){
  const remote=await import(`${BASE}refm.js?v=latest`);
  const callerLocate=options.locateFile;
  return remote.default({...options,locateFile:p=>p.endsWith('.wasm')?`${BASE}refm.wasm`:callerLocate?callerLocate(p):`${BASE}${p}`});
}
