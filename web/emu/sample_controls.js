// Browser-only A/B control for bundled audition samples.
// This module intentionally has no musical state of its own; it calls the WASM engine directly.
export function installSampleControls(mod) {
  const host = document.querySelector('.top');
  if (!host || !mod?._refm_wasm_enable_samples) return;
  const box = document.createElement('div');
  box.style.cssText = 'display:flex;gap:6px;align-items:center;flex-wrap:wrap';
  const title = document.createElement('span');
  title.textContent = 'DRUM SOUND';
  title.style.cssText = 'font-size:11px;color:#999;letter-spacing:.08em';
  box.appendChild(title);
  for (const [track, name] of [[0, '808'], [1, '909']]) {
    const mask = mod._refm_wasm_sample_mask(track) >>> 0;
    const button = document.createElement('button');
    button.className = 'btn';
    button.dataset.sampleTrack = String(track);
    button.dataset.enabled = '0';
    button.textContent = `${name}: SYNTH`;
    button.title = mask ? `Sample lanes available: 0x${mask.toString(16)}` : 'No bundled samples in this build';
    if (!mask) {
      button.disabled = true;
      button.style.opacity = '.45';
    } else {
      button.addEventListener('click', () => {
        const enabled = button.dataset.enabled !== '1';
        mod._refm_wasm_enable_samples(track, enabled ? 1 : 0);
        button.dataset.enabled = enabled ? '1' : '0';
        button.textContent = `${name}: ${enabled ? 'SAMPLE' : 'SYNTH'}`;
        button.classList.toggle('primary', enabled);
      });
    }
    box.appendChild(button);
  }
  host.appendChild(box);
}
