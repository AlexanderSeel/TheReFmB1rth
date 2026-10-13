// SPDX-License-Identifier: GPL-3.0-only
export class ReFmWorkletClient {
  constructor(ctx,mod,frames=512){this.ctx=ctx;this.mod=mod;this.frames=frames;this.ptr=mod._malloc(frames*8);this.node=null;this.rendering=false;this.underflows=0;}
  async init(){
    const workletUrl=new URL('./app/refm_audio_worklet.js',import.meta.url);
    await this.ctx.audioWorklet.addModule(workletUrl);
    this.node=new AudioWorkletNode(this.ctx,'refm-audio',{outputChannelCount:[2]});
    this.node.port.onmessage=e=>{if(e.data?.type==='need'){this.underflows=e.data.underflows||0;this.fill(4);window.dispatchEvent(new CustomEvent('refm-worklet-stats',{detail:e.data}));}};
    this.node.connect(this.ctx.destination);this.fill(6);return this;
  }
  fill(count){if(this.rendering||!this.node)return;this.rendering=true;const started=performance.now();let rendered=0;try{for(let n=0;n<count;n++){this.mod._refm_wasm_render(this.ptr,this.frames);rendered+=this.frames;const base=this.ptr>>2,heap=this.mod.HEAP32,left=new Float32Array(this.frames),right=new Float32Array(this.frames);for(let i=0;i<this.frames;i++){left[i]=heap[base+i*2]/32768;right[i]=heap[base+i*2+1]/32768;}this.node.port.postMessage({type:'block',left,right},[left.buffer,right.buffer]);}}finally{this.rendering=false;window.dispatchEvent(new CustomEvent('refm-worklet-stats',{detail:{underflows:this.underflows,rendered_frames:rendered,render_ms:performance.now()-started}}));}}
  destroy(){if(this.node)this.node.disconnect();if(this.ptr)this.mod._free(this.ptr);this.node=null;this.ptr=0;}
}
