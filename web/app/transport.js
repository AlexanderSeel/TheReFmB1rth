const STATUS = document.querySelector('#transportStatus');
const BUTTONS = [...document.querySelectorAll('[data-transport]')];
let access = null;
let output = null;

function setStatus(text, playing = false) {
  if (!STATUS) return;
  STATUS.textContent = text;
  STATUS.classList.toggle('online', playing);
}

function candidates(midi) {
  return [...midi.outputs.values()].filter(port =>
    port.state !== 'disconnected' && /fm-1|felucca|composite|sinco|usb-midi/i.test(port.name || '')
  );
}

async function ensureOutput() {
  if (!navigator.requestMIDIAccess) throw new Error('Web MIDI is unavailable. Use current Chrome or Edge on HTTPS/localhost.');
  if (!access) {
    access = await navigator.requestMIDIAccess({ sysex: true });
    access.onstatechange = () => { if (output?.state === 'disconnected') { output = null; setStatus('TRANSPORT DISCONNECTED'); } };
  }
  if (output && output.state !== 'disconnected') return output;
  const ports = candidates(access);
  if (ports.length !== 1) throw new Error(ports.length ? `Found ${ports.length} possible FM-1 MIDI outputs. Disconnect all but one.` : 'No FM-1 MIDI output found.');
  output = ports[0]; await output.open(); return output;
}

async function sendRealtime(status, label) {
  try {
    if (window.refmVirtual?.routeActive) {
      window.refmVirtual.sendMidi([status]);
      setStatus(`${label} · VIRTUAL FM-1`, status !== 0xFC);
      return;
    }
    const port = await ensureOutput(); port.send([status]); setStatus(`${label} · ${port.name}`, status !== 0xFC);
  } catch (error) { setStatus('TRANSPORT OFFLINE'); alert(error.message); }
}

for (const button of BUTTONS) button.addEventListener('click', () => {
  const command = button.dataset.transport;
  if (command === 'start') sendRealtime(0xFA, 'PLAYING');
  else if (command === 'continue') sendRealtime(0xFB, 'PLAYING');
  else if (command === 'stop') sendRealtime(0xFC, 'STOPPED');
});
