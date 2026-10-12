const DEVICE_RE=/fm-1|felucca|composite|sinco|usb-midi/i;
const FIELD={machine:108,page:109,pattern:110,playing:111,step:112,lane:113,bpmLo:114,bpmHi:115,p0:116,p1:117,p2:118,p3:119,flags:120,stepType:121,prob:122,drumState:123,project:124};

function candidates(map){return [...map.values()].filter(p=>p.state!=='disconnected'&&DEVICE_RE.test(p.name||''));}
function clamp(v,lo,hi){return Math.max(lo,Math.min(hi,Number(v)||0));}
function setRange(cc,value){document.querySelectorAll(`input[data-cc="${cc}"]`).forEach(el=>{el.value=String(value);const out=el.nextElementSibling;if(out)out.textContent=String(value);});}
function setToggle(cc,on){document.querySelectorAll(`[data-toggle-cc="${cc}"]`).forEach(el=>el.classList.toggle('active',!!on));}

function applySnapshot(ctx,s){
  const {state,refreshSequencer}=ctx;
  state.machine=clamp(s.machine,0,3);
  document.querySelectorAll('.machine').forEach(b=>b.classList.toggle('active',Number(b.dataset.machine)===state.machine));
  const acid=state.machine<2;
  document.querySelector('#acidPanel')?.classList.toggle('hidden',!acid);
  document.querySelector('#drumPanel')?.classList.toggle('hidden',acid);
  const names=['ACID A','ACID B','808','909'];
  if(document.querySelector('#acidName'))document.querySelector('#acidName').textContent=names[state.machine];
  if(document.querySelector('#drumName'))document.querySelector('#drumName').textContent=names[state.machine];
  if(document.querySelector('#mixTrackName'))document.querySelector('#mixTrackName').textContent=names[state.machine];

  state.pattern=clamp(s.pattern,0,7);
  document.querySelectorAll('#patterns button').forEach((b,i)=>b.classList.toggle('active',i===state.pattern));
  state.selectedStep=clamp(s.step,0,15);

  if(s.page===1&&acid){
    setRange(74,s.p0);setRange(71,s.p1);setRange(1,s.p2);setRange(73,s.p3);
    document.querySelectorAll('[data-wave]').forEach(b=>b.classList.toggle('active',Number(b.dataset.wave)===(s.flags&1?1:0)));
  }else if(s.page===2&&acid){
    const st=state.acidPatterns[state.machine][state.pattern][state.selectedStep];
    st.note=s.p0;st.type=clamp(s.stepType,0,2);st.prob=clamp(s.prob,0,100);st.accent=!!(s.flags&2);st.slide=!!(s.flags&4);
  }else if(s.page===3&&!acid){
    state.selectedVoice=clamp(s.lane,0,10);
    state.drumPatterns[state.machine-2][state.pattern][state.selectedStep][state.selectedVoice]=clamp(s.drumState,0,2);
  }else if(s.page===4){
    setRange(7,s.p0);setRange(10,s.p1);setRange(94,s.p2);setRange(91,s.p3);
    setToggle(120,!!(s.flags&8));setToggle(119,!!(s.flags&16));
  }
  refreshSequencer();
}

export function installReadback(ctx){
  const connect=document.querySelector('#connectEditor');
  if(!connect)return;
  const btn=document.createElement('button');
  btn.id='syncEditor';btn.className='secondary';btn.textContent='SYNC FROM FM-1';
  connect.insertAdjacentElement('afterend',btn);
  let input=null,collect=null;

  function onMidi(e){
    const d=[...e.data];
    if(d.length<3||(d[0]&0xF0)!==0xB0||(d[0]&0x0F)!==0x0F)return;
    const cc=d[1]&0x7F,val=d[2]&0x7F;
    if(cc===107&&val===1){collect={};return;}
    if(!collect)return;
    if(cc===107&&val===127){
      const s={};for(const [k,n] of Object.entries(FIELD))s[k]=collect[n]??0;
      s.bpm=(s.bpmLo|s.bpmHi<<7);
      collect=null;applySnapshot(ctx,s);ctx.setMidiStatus(`SYNCED · ${s.bpm} BPM`,true);btn.textContent='SYNCED';setTimeout(()=>btn.textContent='SYNC FROM FM-1',900);return;
    }
    collect[cc]=val;
  }

  btn.addEventListener('click',async()=>{
    btn.disabled=true;btn.textContent='SYNCING…';
    try{
      const access=await ctx.ensureMidi();
      const ins=candidates(access.inputs),outs=candidates(access.outputs);
      if(ins.length!==1||outs.length!==1)throw new Error(`Need exactly one FM-1 MIDI input/output (found ${ins.length}/${outs.length}).`);
      if(input!==ins[0]){if(input)input.removeEventListener('midimessage',onMidi);input=ins[0];await input.open();input.addEventListener('midimessage',onMidi);}
      await outs[0].open();collect=null;outs[0].send([0xBF,107,127]);
      setTimeout(()=>{if(btn.disabled){btn.disabled=false;if(btn.textContent==='SYNCING…')btn.textContent='SYNC FROM FM-1';}},1200);
    }catch(err){ctx.setMidiStatus('SYNC FAILED');btn.textContent='SYNC FROM FM-1';alert(err.message);btn.disabled=false;}
  });
}
