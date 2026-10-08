// Browser hybrid drum source controls. No shadow musical state: all truth comes from WASM.
const NAMES=['BD','SD','CP','RS','CH','OH','LT','MT','HT','CR','RD'];
function bit(mask,lane){return !!(mask&(1<<lane));}
export function installSampleControls(mod){
  if(!mod?._refm_wasm_sample_mask||!mod?._refm_wasm_set_sample_lane)return;
  const refresh=(track,root)=>{
    const avail=mod._refm_wasm_sample_mask(track)>>>0;
    const use=mod._refm_wasm_sample_use_mask(track)>>>0;
    const active=mod._refm_wasm_sample_active_mask(track)>>>0;
    root.querySelectorAll('[data-lane]').forEach(b=>{
      const lane=+b.dataset.lane,a=bit(avail,lane),u=bit(use,lane),on=bit(active,lane);
      b.disabled=!a;b.classList.toggle('sample-active',on);b.classList.toggle('sample-selected',u&&!on);b.classList.toggle('fallback',!a);
      b.textContent=`${NAMES[lane]} · ${on?'SAMPLE':a&&u?'READY':'SYNTH'}`;
      b.title=a?(on?'Bundled sample is active':'Sample available; click to select'):'No bundled sample for this lane; synthesis fallback';
    });
    const c=root.querySelector('[data-count]');if(c)c.textContent=`${NAMES.filter((_,i)=>bit(active,i)).length} sample lanes active`;
  };
  document.querySelectorAll('.drums').forEach((host,track)=>{
    const panel=document.createElement('div');panel.className='refm-source-panel';panel.dataset.sourceTrack=track;
    panel.innerHTML=`<div class="refm-source-head"><b>${track?'909':'808'} SOURCES</b><span data-count></span><button class="refm-source-all">HYBRID ALL</button><button class="refm-source-synth">SYNTH ALL</button></div><div class="refm-source-lanes"></div>`;
    const lanes=panel.querySelector('.refm-source-lanes');
    for(let lane=0;lane<NAMES.length;lane++){
      const b=document.createElement('button');b.className='refm-source-lane';b.dataset.lane=lane;
      b.onclick=()=>{const use=mod._refm_wasm_sample_use_mask(track)>>>0;mod._refm_wasm_set_sample_lane(track,lane,bit(use,lane)?0:1);mod._refm_wasm_enable_samples(track,1);refresh(track,panel);};lanes.appendChild(b);
    }
    panel.querySelector('.refm-source-all').onclick=()=>{mod._refm_wasm_enable_samples(track,1);mod._refm_wasm_set_sample_use_mask(track,0x7ff);refresh(track,panel);};
    panel.querySelector('.refm-source-synth').onclick=()=>{mod._refm_wasm_set_sample_use_mask(track,0);refresh(track,panel);};
    host.appendChild(panel);refresh(track,panel);
  });
}
