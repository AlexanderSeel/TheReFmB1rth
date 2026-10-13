// SPDX-License-Identifier: GPL-3.0-only

const HELP_STYLE=`
.virtualHelp{margin-top:12px}.virtualHelp details{border:1px solid #2d3945;border-radius:10px;background:#0b1117;overflow:hidden}.virtualHelp summary{cursor:pointer;padding:14px 16px;font:800 14px ui-monospace,monospace;letter-spacing:.04em;color:#edf4f8;background:#131b23;list-style:none}.virtualHelp summary::-webkit-details-marker{display:none}.virtualHelp summary::after{content:'+';float:right;color:#49e391}.virtualHelp details[open]>summary::after{content:'−'}.virtualHelpBody{padding:16px}.virtualHelpIntro{display:grid;grid-template-columns:1.2fr .8fr;gap:12px;margin-bottom:14px}.virtualHelpCallout{padding:12px;border:1px solid #315542;border-radius:8px;background:#0d1c15;color:#ccebd9}.virtualHelpGrid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.virtualHelpBlock{border:1px solid #26323c;border-radius:9px;padding:13px;background:#0a1016}.virtualHelpBlock h4{margin:0 0 8px;font:800 12px ui-monospace,monospace;color:#ffca54;letter-spacing:.04em}.virtualHelpBlock p{margin:7px 0;color:#9fb0bc}.virtualHelpTable{width:100%;border-collapse:collapse;font-size:12px}.virtualHelpTable th,.virtualHelpTable td{text-align:left;vertical-align:top;padding:7px 8px;border-top:1px solid #24303a}.virtualHelpTable th{color:#8194a3;font:700 10px ui-monospace,monospace}.virtualHelpTable td:first-child{white-space:nowrap;color:#eef5f8;font:700 11px ui-monospace,monospace}.virtualHelp code{color:#71d5ff;background:#101b24;border:1px solid #253746;border-radius:4px;padding:1px 4px}.virtualHelpKeys{display:flex;flex-wrap:wrap;gap:6px}.virtualHelpKey{padding:5px 7px;border:1px solid #3b4955;border-radius:5px;background:#151e27;color:#dce8ef;font:700 10px ui-monospace,monospace}.virtualHelpNote{margin-top:12px;color:#71808b;font-size:11px}@media(max-width:800px){.virtualHelpIntro,.virtualHelpGrid{grid-template-columns:1fr}.virtualHelpTable{font-size:11px}}
`;

const HELP_HTML=`
<article class="card virtualHelp" id="virtualHelp">
  <details open>
    <summary>VIRTUAL FM-1 HELP / README</summary>
    <div class="virtualHelpBody">
      <div class="virtualHelpIntro">
        <div>
          <div class="eyebrow">HARDWARE-PARITY WORKFLOW</div>
          <h3>Learn the instrument here before using the physical FM-1</h3>
          <p>The virtual unit uses the same <code>ui240</code> state, keyboard-performance mappings and contextual actions as the hardware target. The goal is that muscle memory learned here transfers directly to the FM-1.</p>
        </div>
        <div class="virtualHelpCallout"><strong>Quick start</strong><br>1. SWITCH ON.<br>2. Press PLAY.<br>3. Use ENV/LFO for ACID A/B or tap ARP for drums.<br>4. Tap SEQ for step editing.<br>5. Hold a modifier and use the FM-1 keyboard for fast actions. A used hold does not also trigger the button's normal tap action.<br>6. Virtual desktop: hold a modifier for ~0.36 s to latch it, then press another button/key/encoder. Click the latched button again to release.</div>
      </div>

      <div class="virtualHelpGrid">
        <section class="virtualHelpBlock">
          <h4>PRIMARY PANEL CONTROLS</h4>
          <table class="virtualHelpTable"><tbody>
            <tr><td>ENV</td><td>Select ACID A.</td></tr>
            <tr><td>LFO</td><td>Select ACID B. The label is reused as a shortcut; this is not an LFO page.</td></tr>
            <tr><td>ARP</td><td>Tap: open drums / cycle 808 and 909. Hold: drum-entry layer.</td></tr>
            <tr><td>SEQ</td><td>Tap: open ACID step editor. Hold: direct note-entry layer.</td></tr>
            <tr><td>GLO</td><td>Tap: open mixer. Hold: mute/solo/machine layer.</td></tr>
            <tr><td>FX</td><td>Tap: open PCF / Delay / Dist / Comp rack. Hold: direct FX layer.</td></tr>
            <tr><td>SEL</td><td>Tap: Song / pattern workflow. Hold: FOCUS navigation.</td></tr>
            <tr><td>SAVE</td><td>Tap: Project page. Hold: direct Pattern A–H selection.</td></tr>
            <tr><td>PLAY</td><td>Start / stop the ReFm sequencer.</td></tr>
            <tr><td>HOME</td><td>Return to the performance home page and escape focused editing.</td></tr>
          </tbody></table>
        </section>

        <section class="virtualHelpBlock">
          <h4>ENCODERS</h4>
          <table class="virtualHelpTable"><tbody>
            <tr><td>KNOB 1–4</td><td>Edit the four parameters shown on the active LCD panel.</td></tr>
            <tr><td>PRESETS</td><td>Pattern normally; selected step in STEP/DRUM; project slot in PROJECT. In FOCUS mode it moves through logical panels.</td></tr>
            <tr><td>ALGORITHM</td><td>Select ACID A / ACID B / 808 / 909. In FOCUS mode it also navigates the focus map.</td></tr>
            <tr><td>SELECT</td><td>BPM normally; contextual navigation while focused.</td></tr>
            <tr><td>OCT− / OCT+</td><td>Machine/context decrement/increment; on PROJECT they LOAD / SAVE.</td></tr>
          </tbody></table>
          <p>On the virtual panel, drag a knob vertically, use the mouse wheel, or focus it and use arrow keys. Turning an encoder while a modifier is held consumes that hold, so the modifier button's normal tap action is not fired on release.</p>
        </section>

        <section class="virtualHelpBlock">
          <h4>HOLD / MODIFIER LAYERS</h4>
          <p><strong>Tap vs hold:</strong> modifier-capable buttons defer their normal action until release. If you use another panel button, keyboard key or encoder during the hold, the gesture is consumed as a modifier and the normal tap action is suppressed.</p>
          <p><strong>Multiple buttons:</strong> the virtual panel tracks all held modifier buttons. The active layer follows the same priority as hardware: <code>SEL &gt; REC &gt; EDIT &gt; SAVE &gt; FX &gt; GLO &gt; ARP &gt; SEQ</code>. Releasing the higher-priority button restores the still-held lower layer. Touch/pen can hold buttons simultaneously. With a desktop mouse, long-press a modifier for about 360 ms to latch it, use the second control, then click the latched button again to release it.</p>
          <table class="virtualHelpTable"><tbody>
            <tr><td>hold SEQ + keys</td><td>Write ACID notes directly into the selected step.</td></tr>
            <tr><td>hold ARP + keys</td><td>Use the keyboard as 808/909 drum pads.</td></tr>
            <tr><td>hold GLO + keys</td><td>Keys 1–4 mute tracks, 5–8 solo tracks, following keys select machines.</td></tr>
            <tr><td>hold FX + keys</td><td>Keys 1–4 jump to PCF, Delay, Dist, Comp. Additional keys trigger performance-safe FX throws.</td></tr>
            <tr><td>hold SAVE + keys</td><td>Direct Pattern A–H selection. While PLAY is running, the requested pattern switches cleanly on the next 16-step boundary; while stopped it changes immediately.</td></tr>
            <tr><td>hold EDIT + keys</td><td>Context edit layer: accent, slide, tie/rest and drum high/accent behavior.</td></tr>
            <tr><td>hold REC + keys</td><td>Live-write into the running step position.</td></tr>
            <tr><td>hold SEL + keys</td><td>FOCUS/ZOOM: direct logical-panel selection.</td></tr>
          </tbody></table>
        </section>

        <section class="virtualHelpBlock">
          <h4>FOCUS / ZOOM MAP</h4>
          <p>FOCUS is logical zoom rather than pixel magnification. Hold <code>SEL</code>, then use the first eight keyboard positions:</p>
          <div class="virtualHelpKeys"><span class="virtualHelpKey">1 ACID</span><span class="virtualHelpKey">2 STEP</span><span class="virtualHelpKey">3 DRUM</span><span class="virtualHelpKey">4 MIXER</span><span class="virtualHelpKey">5 PCF</span><span class="virtualHelpKey">6 DELAY</span><span class="virtualHelpKey">7 DIST</span><span class="virtualHelpKey">8 COMP</span></div>
          <p>The RB-240 header shows an eight-position cyan focus map; the active dot is the current logical area. Use PRESETS / ALGORITHM to move between areas. HOME exits immediately.</p>
        </section>

        <section class="virtualHelpBlock">
          <h4>808 KEYBOARD MAP</h4>
          <p>First 11 keyboard positions:</p>
          <div class="virtualHelpKeys"><span class="virtualHelpKey">BD</span><span class="virtualHelpKey">SD</span><span class="virtualHelpKey">RS</span><span class="virtualHelpKey">CP</span><span class="virtualHelpKey">CH</span><span class="virtualHelpKey">OH</span><span class="virtualHelpKey">LT</span><span class="virtualHelpKey">MT</span><span class="virtualHelpKey">HT</span><span class="virtualHelpKey">CR</span><span class="virtualHelpKey">RD</span></div>
          <p>The second 11-key bank writes the same voices with high/accent velocity.</p>
        </section>

        <section class="virtualHelpBlock">
          <h4>909 KEYBOARD MAP</h4>
          <p>The 909 intentionally has a different pad order:</p>
          <div class="virtualHelpKeys"><span class="virtualHelpKey">BD</span><span class="virtualHelpKey">SD</span><span class="virtualHelpKey">LT</span><span class="virtualHelpKey">MT</span><span class="virtualHelpKey">HT</span><span class="virtualHelpKey">RS</span><span class="virtualHelpKey">CP</span><span class="virtualHelpKey">CH</span><span class="virtualHelpKey">OH</span><span class="virtualHelpKey">CR</span><span class="virtualHelpKey">RD</span></div>
          <p>The second bank is HIGH/accent. Extra upper keys provide 909-specific FLAM toggle and flam-width adjustment.</p>
        </section>

        <section class="virtualHelpBlock">
          <h4>COMPUTER KEYBOARD SHORTCUTS</h4>
          <table class="virtualHelpTable"><tbody>
            <tr><td>Q W E R T Y U I</td><td>First eight white FM-1 keys.</td></tr>
            <tr><td>A S D F G H J K</td><td>Next eight white FM-1 keys.</td></tr>
            <tr><td>1–0, −</td><td>Black FM-1 keys.</td></tr>
            <tr><td>Space</td><td>PLAY / STOP.</td></tr>
            <tr><td>Enter</td><td>REC contextual action.</td></tr>
            <tr><td>Esc</td><td>HOME.</td></tr>
            <tr><td>Z / X</td><td>OCT− / OCT+ behavior.</td></tr>
            <tr><td>↑ / ↓</td><td>SELECT encoder / BPM.</td></tr>
          </tbody></table>
        </section>

        <section class="virtualHelpBlock">
          <h4>PROJECT / TESTING NOTES</h4>
          <p><strong>SAVE VIRTUAL</strong> stores the complete virtual project in browser local storage. The PROJECT page uses the same OCT− LOAD / OCT+ SAVE interaction model as hardware.</p>
          <p><strong>FACTORY RESET</strong> clears the browser-local virtual project and restarts the virtual engine. It does not touch a physical FM-1.</p>
          <p>Use the LIVE EDITOR's <strong>USE VIRTUAL</strong> route to verify editor → panel → sound behavior before a physical firmware test.</p>
        </section>
      </div>
      <p class="virtualHelpNote">This help describes the current unreleased post-beta.2 interaction work on <code>main</code>. The published beta.2 firmware remains unchanged.</p>
    </div>
  </details>
</article>`;

export function installVirtualHelp(){
  const page=document.querySelector('#virtual');
  if(!page||document.querySelector('#virtualHelp'))return;
  if(!document.querySelector('#virtual-help-style')){const style=document.createElement('style');style.id='virtual-help-style';style.textContent=HELP_STYLE;document.head.appendChild(style);}
  page.insertAdjacentHTML('beforeend',HELP_HTML);
}

installVirtualHelp();
