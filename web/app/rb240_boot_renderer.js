// SPDX-License-Identifier: GPL-3.0-only
// Hardware-feasible Virtual FM-1 startup animation. The renderer keeps the
// 240x240 logical UI coordinate system, but can rasterize it directly into a
// 480x480 backing canvas so 2x mode uses four times as many pixels instead of
// scaling a previously rendered 240x240 bitmap.
import {drawRB240 as drawMain} from './rb240_renderer_v2.js';

const C={bg:'#111416',metal:'#777b7b',metalHi:'#d4d5d1',metalLo:'#34393b',dark:'#202426',ink:'#111213',text:'#f4f1e8',red:'#ff382f',amber:'#ff9a27',cyan:'#27bdcf',green:'#55e36b',off:'#41484a'};
let wasReady=false,bootStart=0;
const now=()=>globalThis.performance?.now?.()??Date.now();
function rect(g,x,y,w,h,fill,stroke=null){g.fillStyle=fill;g.fillRect(x,y,w,h);if(stroke){g.strokeStyle=stroke;g.strokeRect(x+.5,y+.5,w-1,h-1);}}
function tx(g,x,y,s,size=8,color=C.text,bold=true,align='left'){g.fillStyle=color;g.font=`${bold?'700 ':'400 '}${size}px Arial Narrow,Arial,sans-serif`;g.textAlign=align;g.textBaseline='alphabetic';g.fillText(String(s),x,y);}
function bevel(g,x,y,w,h,fill=C.metal){rect(g,x,y,w,h,fill);rect(g,x,y,w,1,C.metalHi);rect(g,x,y,1,h,C.metalHi);rect(g,x,y+h-1,w,1,C.metalLo);rect(g,x+w-1,y,1,h,C.metalLo);}
function led(g,x,y,on,color=C.amber){rect(g,x,y,7,7,C.ink);rect(g,x+1,y+1,5,5,on?color:C.off);}
function bootFrame(g,ms){
  const phase=Math.max(0,Math.min(5,Math.floor(ms/260)));
  const steps=Math.max(0,Math.min(16,Math.floor((ms-520)/48)+1));
  rect(g,0,0,240,240,C.bg);
  bevel(g,7,7,226,226,C.metalLo);
  bevel(g,12,12,216,216,C.metal);
  rect(g,16,16,208,26,C.dark,C.ink);
  tx(g,22,34,'THE',7,C.text,true);
  tx(g,43,34,'ReFM',14,C.red,true);
  tx(g,84,34,'B1RTH',14,C.amber,true);
  tx(g,217,33,'RB-240',7,C.metalHi,true,'right');

  rect(g,18,49,204,20,C.dark,C.ink);
  tx(g,23,62,'M-VAVE FM-1 / REBORN GROOVE SYSTEM',6,C.text,true);

  const names=['ACID A','ACID B','808','909'];
  const colors=[C.red,C.red,C.amber,C.amber];
  for(let i=0;i<4;i++){
    const y=78+i*25,on=phase>=i+1;
    bevel(g,19,y,202,20,on?'#929593':'#626768');
    led(g,25,y+6,on,colors[i]);
    tx(g,39,y+14,names[i],8,on?C.ink:C.metalHi,true);
    tx(g,211,y+14,on?'ONLINE':'WAIT',6,on?C.ink:C.metalHi,true,'right');
  }

  rect(g,18,183,204,30,C.dark,C.ink);
  tx(g,21,192,'16-STEP BUS',5,C.metalHi,true);
  for(let i=0;i<16;i++){
    const x=21+i*12.3,on=i<steps,beat=(i%4)===0;
    rect(g,x,198,9,9,C.ink);
    rect(g,x+1,199,7,7,on?(phase>=5&&beat?C.green:C.amber):C.off);
  }
  const status=phase===0?'PANEL BUS':phase===1?'ACID A ENGINE':phase===2?'ACID B ENGINE':phase===3?'DRUM VOICES':phase===4?'MIXER + FX':'READY';
  rect(g,18,217,204,8,C.dark);
  tx(g,120,224,status,6,phase>=5?C.green:C.cyan,true,'center');
}

function logicalRender(g,draw){
  if(!g)return;
  const w=g.canvas?.width||240,h=g.canvas?.height||240;
  const sx=w/240,sy=h/240;
  g.save();
  g.setTransform(sx,0,0,sy,0,0);
  try{return draw();}finally{g.restore();}
}

function normalized303Get(get){
  return key=>{
    const raw=get(key);
    if(typeof key==='number'&&key>=36&&key<52){
      const packed=Number(raw)>>>0,note=packed&255;
      if(note<24||note>60)return (packed&0xffffff00)|36;
    }
    return raw;
  };
}

export function drawRB240(g,get,projectResult='READY'){
  const ready=!!get('ready'),viewGet=normalized303Get(get);
  if(!ready){wasReady=false;bootStart=0;return logicalRender(g,()=>drawMain(g,viewGet,projectResult));}
  if(!wasReady){wasReady=true;bootStart=now();}
  const elapsed=now()-bootStart;
  if(elapsed<1560)return logicalRender(g,()=>{g.imageSmoothingEnabled=false;bootFrame(g,elapsed);});
  return logicalRender(g,()=>drawMain(g,viewGet,projectResult));
}
