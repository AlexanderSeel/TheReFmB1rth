// AudioWorklet sink for TheReFmB1rth WASM validation.
// DSP blocks are supplied over MessagePort; the realtime callback only consumes buffered Float32 audio.
class ReFmAudioProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.queue = [];
    this.offset = 0;
    this.underflows = 0;
    this.port.onmessage = (e) => {
      if (e.data?.type === 'block' && e.data.left && e.data.right) {
        this.queue.push({ left: e.data.left, right: e.data.right });
      }
    };
  }
  process(inputs, outputs) {
    const out = outputs[0];
    if (!out?.length) return true;
    const l = out[0], r = out[1] || out[0];
    let i = 0;
    while (i < l.length) {
      const block = this.queue[0];
      if (!block) {
        l.fill(0, i); r.fill(0, i);
        this.underflows++;
        break;
      }
      const available = block.left.length - this.offset;
      const take = Math.min(available, l.length - i);
      l.set(block.left.subarray(this.offset, this.offset + take), i);
      r.set(block.right.subarray(this.offset, this.offset + take), i);
      i += take; this.offset += take;
      if (this.offset >= block.left.length) { this.queue.shift(); this.offset = 0; }
    }
    if (this.queue.length < 2) this.port.postMessage({ type: 'need', queued: this.queue.length, underflows: this.underflows });
    return true;
  }
}
registerProcessor('refm-audio', ReFmAudioProcessor);
