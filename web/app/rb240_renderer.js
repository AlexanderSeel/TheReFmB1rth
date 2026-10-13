// SPDX-License-Identifier: GPL-3.0-only
// Canvas mirror of the physical 240x240 beta renderer. Keep geometry aligned
// with tools/apply_rb240_beta3_layout.py.

const C={
  bg:'#17191b', bezel:'#090a0b', panel:'#a9aaa7', panel2:'#666866', dark:'#252729',
  black:'#101112', light:'#deded9', ink:'#141414', text:'#f5f4ec', dim:'#c9c8c1',
  led:'#240304', red:'#ff2929', amber:'#ffb51b', orange:'#ff7a16', yellow:'#ffe01d',
  cyan:'#21b6c9', green:'#50e25a'
};
const PAGE=['HOME','ACID','SEQ EDIT','DRUM','MIXER','FX','SONG','PROJECT','EXT'];
const MACHINE=['ACID A','ACID B','808','909'];
const D808=['BD','SD','CP','RS','CH','OH','LT','MT','HT','CY','CB'];
const D909=['BD','SD','CP','RS','CH','OH','LT','MT','HT','CR','RD'];
const FX=['PCF','DELAY','DIST','COMP'];

function tx(g,x,y,s,size=8,color=C.ink,bold=true,align='left'){
  g.fillStyle=color; g.font=`${bold?'700 ':'400 '}${size}px Arial Narrow,Arial,sans-serif`;
  g.textAlign=align; g.textBaseline='alphabetic'; g.fillText(String(s),x,y);
}
function rect(g,x,y,w,h,fill,stroke){g.fillStyle=fill;g.fillRect(x,y,w,h);if(stroke){g.strokeStyle=stroke;g.strokeRect(x+.5,y+.5,w-1,h-1);}}
function screw(g,x,y){g.fillStyle=C.dark;g.fillRect(x,y,5,5);g.fillStyle=C.panel2;g.fillRect(x+1,y+1,3,3);g.fillStyle=C.light;g.fillRect(x+2,y,1,5);g.fillRect(x,y+2,5,1);}
function panel(g,x,y,w,h,fill=C.panel){rect(g,x,y,w,h,fill);g.fillStyle=C.light;g.fillRect(x,y,w,1);g.fillRect(x,y,1,h);g.fillStyle=C.dark;g.fillRect(x,y+h-1,w,1);g.fillRect(x+w-1,y,1,h);if(w>25&&h>17){screw(g,x+3,y+3);screw(g,x+w-8,y+3);}}
function ledWell(g,x,y,w,h,value,size=11,color=C.red){rect(g,x,y,w,h,C.led,C.black);tx(g,x+w/2,y+h-4,value,size,color,true,'center');}
function lamp(g,x,y,on,color=C.red){rect(g,x,y,6,6,C.black);rect(g,x+1,y+1,4,4,on?color:'#4a1111');}
function button(g,x,y,w,h,label,on=false,color=C.amber){rect(g,x,y,w,h,C.dark);rect(g,x+2,y+2,w-4,h-4,on?color:C.panel2,C.light);if(label)tx(g,x+w/2,y+h/2+3,label,6,on?C.ink:C.text,true,'center');}
function band(g,y,label,color=C.amber){rect(g,4,y,232,11,color,C.dark);tx(g,12,y+8,label,7,C.ink,true);screw(g,6,y+3);screw(g,228,y+3);}
function card(g,x,y,w,label,value,opts={}){panel(g,x,y,w,36);tx(g,x+5,y+10,label,6,C.ink,true);lamp(g,x+w-10,y+4,!!opts.lamp,opts.lampColor||C.red);ledWell(g,x+5,y+15,w-10,17,value,opts.size||9,opts.color||C.red);}
function knob(g,x,y,label,val=50,color=C.orange){tx(g,x+8,y-3,label,6,C.ink,true,'center');g.fillStyle=C.black;g.beginPath();g.arc(x+8,y+8,7,0,Math.PI*2);g.fill();g.strokeStyle=C.light;g.beginPath();g.arc(x+8,y+8,6,0,Math.PI*2);g.stroke();const a=(-135+Math.max(0,Math.min(100,val))*270/100)*Math.PI/180;g.strokeStyle=color;g.lineWidth=2;g.beginPath();g.moveTo(x+8,y+8);g.lineTo(x+8+Math.cos(a)*5,y+8+Math.sin(a)*5);g.stroke();g.lineWidth=1;}
function meter(g,x,y,h,p){rect(g,x,y,8,h,C.black);const seg=4,n=Math.floor((h-2)/seg),on=Math.round(n*Math.max(0,Math.min(100,p))/100);for(let i=0;i<n;i++){const sy=y+h-3-i*seg;let c=i>=n-2?C.red:i>=n-4?C.yellow:C.green;rect(g,x+2,sy,4,2,i<on?c:'#263126');}}
function footer(g,s){panel(g,3,211,234,26,C.panel2);rect(g,7,217,226,14,C.dark);tx(g,10,227,s,6,C.text,false);}
function header(g,s){
  panel(g,2,2,236,23);rect(g,5,5,58,17,C.amber,C.dark);tx(g,34,18,PAGE[s.page]||'REFM',10,C.ink,true,'center');
  tx(g,70,18,MACHINE[s.machine]||'?',9,C.ink,true);ledWell(g,133,5,48,17,s.bpm,11,C.red);rect(g,184,5,51,17,C.dark);tx(g,209,17,s.playing?'PLAY':'STOP',7,s.playing?C.green:C.text,true,'center');
  panel(g,2,27,236,22);tx(g,7,41,'PATTERN',6,C.ink,true);ledWell(g,47,31,18,15,String.fromCharCode(65+(s.pattern&7)),8,C.red);button(g,68,31,18,15,'<');button(g,88,31,18,15,'>');tx(g,109,37,'PRESET',5,C.ink);tx(g,153,37,'ALGO',5,C.ink);tx(g,195,37,'SELECT',5,C.ink);lamp(g,124,40,true);lamp(g,164,40,false);lamp(g,211,40,false);
}
function stepPads(g,s,y=171){for(let i=0;i<16;i++){const x=5+i*14.4;tx(g,x+6,y-3,i+1,4,C.text,true,'center');button(g,x,y,12,18,'',i===s.selectedStep?true:(s.playing&&i===s.playStep),i===s.selectedStep?C.red:C.green);}}
function home(g,s){band(g,51,'SEQUENCER');card(g,4,64,56,'TRACK',MACHINE[s.machine]);card(g,62,64,56,'PATTERN',String.fromCharCode(65+(s.pattern&7)));card(g,120,64,56,'STEP',s.playStep+1);card(g,178,64,58,'STATE',s.playing?'PLAY':'STOP');button(g,4,103,34,20,'PREV');button(g,40,103,34,20,'NEXT');button(g,76,103,34,20,'PLAY',s.playing,C.green);button(g,112,103,34,20,'STOP',!s.playing,C.red);card(g,150,101,40,'BAR','001',{size:7});card(g,192,101,44,'LENGTH','016',{size:7});panel(g,4,130,146,34);tx(g,10,141,'TRACK MUTE',6,C.ink);for(let i=0;i<8;i++)button(g,9+i*17,145,14,14,'',i===s.machine,C.amber);panel(g,154,130,82,34);tx(g,160,141,'LOOP',6,C.ink);lamp(g,160,147,true);knob(g,177,145,'START',20);knob(g,207,145,'LENGTH',70);panel(g,4,170,232,27,C.panel2);rect(g,8,175,224,16,C.dark);tx(g,11,186,'Keys play selected ACID track',6,C.text,false);footer(g,'ENV=A  LFO=B  ARP=DRUM  SEQ=STEP');}
function acid(g,s){band(g,51,'ACID CONTROL');const vals=[s.cutoff,s.res,s.env,s.decay];const names=['CUTOFF','RESO','ENV.MOD','DECAY'];for(let i=0;i<4;i++){panel(g,4+i*58,64,56,70);knob(g,23+i*58,82,names[i],vals[i]);ledWell(g,9+i*58,109,46,18,vals[i],8,C.red);}panel(g,4,139,232,58);tx(g,10,150,'WAVEFORM',6,C.ink);button(g,10,156,28,19,'SAW',!s.wave,C.amber);button(g,40,156,32,19,'SQR',!!s.wave,C.amber);tx(g,82,150,'TUNE',6,C.ink);ledWell(g,80,156,30,19,s.tune,7,C.red);tx(g,119,150,'ACCENT',6,C.ink);ledWell(g,117,156,34,19,s.acidAccent,7,C.red);button(g,158,156,34,19,'STEP');button(g,195,156,36,19,'KEYS',true,C.green);footer(g,'K1 CUT  K2 RES  K3 ENV  K4 DEC | EDIT WAVE');}
function seq(g,s){
  band(g,51,'SEQ EDIT');
  panel(g,4,64,232,47);
  tx(g,9,75,'WAVE',5,C.ink);button(g,8,79,26,16,s.wave?'SQR':'SAW',true,C.amber);
  const kv=[s.tune+64,s.cutoff,s.res,s.env,s.decay,s.acidAccent], kn=['TUNE','CUTOFF','RESO','ENV','DECAY','ACCENT'];
  for(let i=0;i<6;i++)knob(g,39+i*31,82,kn[i],i===0?Math.max(0,Math.min(100,(s.tune+24)*100/48)):kv[i]);
  panel(g,4,114,232,51);
  rect(g,7,118,35,42,C.dark);tx(g,24,130,'PITCH',6,C.text,true,'center');tx(g,24,139,'MODE',6,C.text,true,'center');lamp(g,20,147,s.stepType===1,C.red);button(g,8,151,32,8,'CLEAR');
  const white=[0,2,4,5,7,9,11];for(let i=0;i<7;i++){const x=46+i*18;button(g,x,130,15,29,'',false,C.panel);lamp(g,x+5,151,(s.note%12)===white[i],C.red);}const black=[1,3,6,8,10];const bx=[57,75,111,129,147];for(let i=0;i<5;i++){rect(g,bx[i],119,11,22,C.black);lamp(g,bx[i]+3,121,(s.note%12)===black[i],C.red);}
  rect(g,174,118,58,18,C.panel);tx(g,180,130,s.stepType===0?'REST':s.stepType===2?'TIE':'NOTE',7,C.ink,true);lamp(g,213,126,s.accent,C.red);
  for(let i=0;i<4;i++){const x=174+i*15;tx(g,x+6,144,['DN','UP','ACC','SLD'][i],4,C.ink,true,'center');button(g,x,147,13,13,'',i===2?s.accent:i===3?s.slide:false,i>=2?C.amber:C.panel);}
  stepPads(g,s,181);
  footer(g,`NOTE ${s.note}  TYPE ${['REST','NOTE','TIE'][s.stepType]||'?'}  PROB ${s.prob}%  ACC ${s.accent?'ON':'OFF'}  SLD ${s.slide?'ON':'OFF'}`);
}
function drums(g,s){
  const is909=s.machine===3, names=is909?D909:D808;
  band(g,51,is909?'TR-909 RHYTHM':'TR-808 RHYTHM');
  panel(g,4,64,184,51);for(let i=0;i<16;i++){const x=8+(i&7)*22,y=70+(i>>3)*22;button(g,x,y,18,17,'',i===s.selectedStep||(s.playing&&i===s.playStep),i===s.selectedStep?C.amber:C.green);}
  panel(g,191,64,45,51);tx(g,213,75,'STEP',6,C.ink,true,'center');ledWell(g,197,79,34,18,s.selectedStep+1,9,C.red);tx(g,197,109,is909?'FLAM':'ACC',5,C.ink,true);lamp(g,222,105,is909?false:s.drumState===2,is909?C.orange:C.red);
  card(g,4,120,45,'LANE',names[s.lane]||'?');card(g,51,120,45,is909?'LEVEL':'STATE',is909?['OFF','LOW','HIGH'][s.drumState]||'OFF':['OFF','HIT','ACC'][s.drumState]||'OFF');card(g,98,120,45,'STEP',s.selectedStep+1);card(g,145,120,43,'KIT',is909?'909':'808');panel(g,191,120,45,77,C.dark);tx(g,213,132,'OUTPUT',5,C.text,true,'center');meter(g,207,139,50,s.drumState?78:15);
  panel(g,4,160,184,37);const labels=is909?['TUNE','ATT','DEC','TONE','SNAP','FLAM']:['TUNE','LEVEL','DECAY','TONE','SNAP','PAN'];for(let i=0;i<6;i++)knob(g,10+i*29,171,labels[i],50,is909&&i===5?C.orange:C.amber);
  footer(g,is909?'K1 LANE  K2 OFF/LOW/HIGH | EDIT/REC FX/FLAM':'K1 LANE  K2 OFF/HIT/ACC | PRESET STEP');
}
function mixer(g,s){band(g,51,'CHANNEL / LEVELS');card(g,4,64,54,'LEVEL',s.level);card(g,60,64,54,'PAN',s.pan);card(g,116,64,54,'DELAY',s.delay);card(g,172,64,54,'REVERB',s.reverb);for(let i=0;i<4;i++){panel(g,4+i*56,102,54,57);knob(g,23+i*56,118,['LEVEL','PAN','DLY','REV'][i],[s.level,s.pan+50,s.delay,s.reverb][i]);}card(g,4,162,54,'MUTE',s.mute?'ON':'OFF',{color:C.amber});card(g,60,162,54,'SOLO',s.solo?'ON':'OFF',{color:C.amber});card(g,116,162,54,'EDIT','MUTE',{color:C.amber});card(g,172,162,54,'REC','SOLO',{color:C.amber});meter(g,229,75,120,s.level);footer(g,'ALGO TRACK | K1 LVL  K2 PAN  K3 DLY  K4 REV');}
function fx(g,s){
  band(g,51,`FX / ${FX[s.fx]||'PCF'}`,C.cyan);const names=['PCF','DELAY','DIST','COMP'];for(let i=0;i<4;i++)button(g,5+i*58,64,54,18,names[i],i===s.fx,C.cyan);
  panel(g,4,86,232,108);
  if(s.fx===0){tx(g,12,99,'PATTERN CONTROL FILTER',7,C.ink,true);card(g,10,105,50,'FREQ',s.fx0);card(g,64,105,50,'MODE','LP');card(g,118,105,50,'AMT',s.fx2);card(g,172,105,50,'DECAY',s.fx3);knob(g,27,158,'FREQ',s.fx0,C.cyan);knob(g,82,158,'Q',35,C.cyan);knob(g,137,158,'AMT',s.fx2,C.cyan);knob(g,192,158,'DEC',s.fx3,C.cyan);}
  else if(s.fx===1){tx(g,12,99,'TEMPO DELAY',7,C.ink,true);card(g,10,105,50,'STEPS',s.fx0);card(g,64,105,50,'PAN',0);card(g,118,105,50,'FEEDBACK',s.fx2);card(g,172,105,50,'MIX',s.fx3);knob(g,27,158,'STEPS',s.fx0*6,C.cyan);knob(g,82,158,'PAN',50,C.cyan);knob(g,137,158,'FB',s.fx2,C.cyan);knob(g,192,158,'MIX',s.fx3,C.cyan);}
  else if(s.fx===2){tx(g,12,99,'DISTORTION',7,C.ink,true);card(g,20,108,85,'AMOUNT',s.fx0);card(g,132,108,85,'SHAPE',s.fx1);knob(g,48,159,'AMOUNT',s.fx0,C.orange);knob(g,160,159,'SHAPE',s.fx1,C.orange);}
  else {tx(g,12,99,'COMPRESSOR',7,C.ink,true);card(g,20,108,85,'THRESH',s.fx0);card(g,132,108,85,'RATIO',`${s.fx1}:1`);knob(g,48,159,'THRESH',s.fx0,C.green);knob(g,160,159,'RATIO',Math.min(100,s.fx1*20),C.green);}
  footer(g,'FX/EDIT NEXT  REC PREV | K1-K4 EDIT ACTIVE MODULE');
}
function song(g,s){band(g,51,'PATTERN / SONG');card(g,4,64,56,'MODE','PATTERN');card(g,62,64,56,'BANK','A-H');card(g,120,64,56,'PLAY',s.playing?'RUN':'STOP');card(g,178,64,58,'EDIT',s.songLoop?'LOOP':'ONCE');panel(g,4,104,232,27,C.panel2);rect(g,8,110,224,15,C.dark);tx(g,11,121,'Pattern playback / chain focus',6,C.text,false);button(g,5,136,35,20,'PREV');button(g,42,136,35,20,'PLAY',s.playing,C.green);button(g,79,136,35,20,'STOP',!s.playing,C.red);button(g,116,136,35,20,'REC');card(g,155,134,38,'BAR','001',{size:7});card(g,195,134,41,'LENGTH',s.songLength||16,{size:7});tx(g,8,177,'CHAIN',6,C.ink,true);for(let i=0;i<16;i++)button(g,6+i*14.3,181,12,17,'',i===(s.pattern&15),C.amber);footer(g,'PRESET PATTERN | PLAY START/STOP | EDIT LOOP');}
function project(g,s){band(g,51,'PROJECT MEMORY');card(g,4,66,54,'SLOT',s.slot+1);card(g,60,66,54,'OCT-','LOAD');card(g,116,66,54,'OCT+','SAVE');card(g,172,66,54,'STATUS',s.projectResult||'READY');panel(g,4,108,232,76);tx(g,12,126,'Project slots use CRC-checked A/B storage.',7,C.ink,true);tx(g,12,143,'PRESET or K1 selects slot.',7,C.ink,true);tx(g,12,160,'OCT- LOAD   OCT+ SAVE',7,C.ink,true);footer(g,'HOME EXIT | SAVE ONLY AFTER AUDITION');}

export function drawRB240(g,get,projectResult='READY'){
  if(!g)return;g.imageSmoothingEnabled=false;rect(g,0,0,240,240,C.bg);
  const ready=!!get('ready');if(!ready){panel(g,12,74,216,90,C.panel2);rect(g,22,92,196,37,C.led);tx(g,120,116,'TheReFmB1rth',15,C.red,true,'center');tx(g,120,147,'SWITCH ON VIRTUAL FM-1',8,C.text,true,'center');return;}
  const v=i=>Number(get(i)||0), page=v(0),machine=v(1),pattern=v(2),playing=v(3),playStep=v(4),selectedStep=v(5),lane=v(6),slot=v(7),bpm=v(8),p0=v(9),p1=v(10),p2=v(11),p3=v(12),wave=v(13),stepType=v(14),accent=v(15),slide=v(16),prob=v(17),drumState=v(18),mute=v(19),solo=v(20),fx=v(22),tune=v(25)-64,acidAccent=v(26),songLoop=v(27),songLength=v(28);
  const pct=x=>Math.max(0,Math.min(100,Math.round(Number(x)*100/32767)));
  const s={page,machine,pattern,playing,playStep,selectedStep,lane,slot,bpm,wave,stepType,accent,slide,prob,drumState,mute,solo,fx,tune,acidAccent,songLoop,songLength,projectResult,
    cutoff:page===1?pct(p0):50,res:page===1?pct(p1):35,env:page===1?pct(p2):50,decay:page===1?pct(p3):50,note:p0,
    level:page===4?pct(p0):100,pan:page===4?Math.round((p1-32767)*100/32767):0,delay:page===4?Math.round(p2*100/127):0,reverb:page===4?Math.round(p3*100/127):0,
    fx0:page===5?(fx===0?pct(p0):p0):0,fx1:page===5?p1:0,fx2:page===5?p2:0,fx3:page===5?p3:0};
  // STEP page needs the synth strip even though p0-p3 carry step values.
  if(page===2){s.cutoff=Number(get(29)||64);s.res=Number(get(30)||32);s.env=Number(get(31)||40);s.decay=Number(get(32)||28);}
  header(g,s);
  if(page===0)home(g,s);else if(page===1)acid(g,s);else if(page===2)seq(g,s);else if(page===3)drums(g,s);else if(page===4)mixer(g,s);else if(page===5)fx(g,s);else if(page===6)song(g,s);else if(page===7)project(g,s);else{band(g,51,'DIAGNOSTICS',C.cyan);panel(g,4,66,232,112);tx(g,12,90,'Reserved for beta diagnostics',8,C.ink,true);footer(g,'HOME returns to performance view');}
}
