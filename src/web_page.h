// The local fallback control page, served at http://<device>/. Self-contained so it works with no internet
// and no Home Assistant. Talks to /api/state and /api/set.
#pragma once

#include <Arduino.h>

static const char kWebPage[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0f1115">
<title>Aircon</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--text:#14161a;--muted:#6b7280;--line:#e5e7eb;--chip:#eef0f3;--accent:#6b7280;--glow:transparent}
@media (prefers-color-scheme:dark){:root{--bg:#0f1115;--card:#181b21;--text:#f2f3f5;--muted:#8b93a1;--line:#262a32;--chip:#22262e}}
body[data-mode=cool]{--accent:#2f8cff;--glow:#2f8cff33}
body[data-mode=heat]{--accent:#ff7a1a;--glow:#ff7a1a33}
body[data-mode=dry]{--accent:#14b8a6;--glow:#14b8a633}
body[data-mode=fan_only]{--accent:#8b5cf6;--glow:#8b5cf633}
body[data-mode=auto]{--accent:#22c55e;--glow:#22c55e33}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;background:var(--bg);color:var(--text);font:15px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;transition:background .4s}
main{max-width:440px;margin:0 auto;padding:20px 16px 40px}
header{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:16px}
h1{font-size:20px;margin:0;font-weight:650}
.sub{color:var(--muted);font-size:13px;display:flex;align-items:center;gap:6px;margin-top:2px}
.dot{width:8px;height:8px;border-radius:50%;background:#9ca3af}.dot.ok{background:#22c55e}.dot.bad{background:#ef4444}
.card{background:var(--card);border:1px solid var(--line);border-radius:22px;padding:18px;margin-bottom:12px}
.hero{text-align:center;padding:28px 18px;box-shadow:0 20px 60px -20px var(--glow);transition:box-shadow .4s}
.temp{display:flex;align-items:center;justify-content:center;gap:22px}
.value{font-size:clamp(56px,19vw,76px);font-weight:300;letter-spacing:-3px;min-width:2.2ch;font-variant-numeric:tabular-nums;transition:color .4s}
.value small{font-size:28px;letter-spacing:0;color:var(--muted);vertical-align:top;margin-left:2px}
body[data-mode=off] .value{color:var(--muted)}
.round{flex:none;width:52px;height:52px;border-radius:50%;border:1px solid var(--line);background:var(--chip);color:var(--text);font-size:26px;cursor:pointer;transition:transform .1s,background .2s}
.round:active{transform:scale(.9)}
.power{width:46px;height:46px;border-radius:50%;border:0;cursor:pointer;background:var(--chip);color:var(--muted);transition:all .3s}
body:not([data-mode=off]) .power{background:var(--accent);color:#fff;box-shadow:0 6px 20px -4px var(--accent)}
.power svg{width:22px;height:22px}
.label{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.08em;margin:0 0 10px}
.chips{display:flex;flex-wrap:wrap;gap:8px}
.chip{flex:1 1 auto;border:1px solid var(--line);background:var(--chip);color:var(--text);border-radius:999px;padding:9px 14px;font:inherit;cursor:pointer;transition:all .2s;text-transform:capitalize}
.chip.on{background:var(--accent);border-color:transparent;color:#fff}
body[data-mode=off] .chip.on{background:var(--text);color:var(--card)}
.row{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.row.three{grid-template-columns:repeat(auto-fit,minmax(130px,1fr))}
select{width:100%;appearance:none;border:1px solid var(--line);background:var(--chip) url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='8'%3E%3Cpath d='M1 1l5 5 5-5' fill='none' stroke='%238b93a1' stroke-width='2'/%3E%3C/svg%3E") no-repeat right 12px center;color:var(--text);border-radius:14px;padding:11px 32px 11px 12px;font:inherit;text-transform:capitalize}
.field span{display:block;color:var(--muted);font-size:12px;margin:0 0 6px 4px}
.banner{display:none;align-items:center;gap:12px;background:var(--accent);color:#fff;border-radius:18px;padding:14px 16px;margin-bottom:12px}
.banner.show{display:flex}
.pulse{width:12px;height:12px;border-radius:50%;background:#fff;animation:p 1.2s infinite}
@keyframes p{0%{box-shadow:0 0 0 0 #fff9}100%{box-shadow:0 0 0 14px #fff0}}
footer{color:var(--muted);font-size:12px;text-align:center;line-height:1.9}
footer a,footer button{color:inherit;background:none;border:0;font:inherit;text-decoration:underline;cursor:pointer;padding:0}
.toast{position:fixed;left:50%;bottom:22px;transform:translate(-50%,120px);background:var(--text);color:var(--card);padding:10px 16px;border-radius:999px;font-size:14px;transition:transform .35s cubic-bezier(.2,.9,.3,1.2)}
.toast.show{transform:translate(-50%,0)}
</style>
</head>
<body data-mode="off">
<main>
<header>
<div><h1 id="name">Aircon</h1><div class="sub"><span id="dot" class="dot"></span><span id="status">Connecting…</span></div></div>
<button class="power" id="power" aria-label="Power"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"><path d="M12 3v8"/><path d="M6.3 7a8 8 0 1 0 11.4 0"/></svg></button>
</header>
<div class="banner" id="learn"><div class="pulse"></div><div><b>Point your remote at me</b><br>and press any button to teach me your aircon.</div></div>
<section class="card hero">
<div class="temp"><button class="round" id="down" aria-label="Colder">−</button><div class="value" id="temp">--<small>°</small></div><button class="round" id="up" aria-label="Warmer">+</button></div>
</section>
<section class="card"><p class="label">Mode</p><div class="chips" id="modes"></div></section>
<section class="card"><div class="row three">
<label class="field"><span>Fan Speed</span><select id="fan_mode" aria-label="Fan speed"></select></label>
<label class="field"><span>Vertical Swing</span><select id="swing_mode" aria-label="Vertical swing"></select></label>
<label class="field"><span>Horizontal Swing</span><select id="swing_horizontal_mode" aria-label="Horizontal swing"></select></label>
</div></section>
<section class="card"><p class="label">Features</p><div class="chips" id="features"></div></section>
<footer><div id="meta"></div><a href="/update">Update firmware</a> · <button id="learnBtn">Learn remote</button> · <button id="restart">Restart</button></footer>
</main>
<div class="toast" id="toast" role="status" aria-live="polite"></div>
<script>
const $=id=>document.getElementById(id);
const FEATURES=[["quiet","Quiet"],["turbo","Turbo"],["econo","Eco"],["sleep","Sleep"],["light","Display Light"],["filter","Air Filter"],["clean","Self-Clean"],["beep","Beep Sounds"]];
const MODE_LABEL={off:"Off",auto:"Auto",cool:"Cool",heat:"Heat",dry:"Dry",fan_only:"Fan"};
// Polls that started before the latest local change would briefly undo it on screen, so they only refresh the meta info.
let s=null,opts=null,changedAt=0,polling=false,tempTimer=null,toastTimer=null,lastLearning=null,lastProtocol=null;
function toast(t){const e=$("toast");e.textContent=t;e.classList.add("show");clearTimeout(toastTimer);toastTimer=setTimeout(()=>e.classList.remove("show"),2200)}
function buzz(){navigator.vibrate&&navigator.vibrate(8)}
function request(url,init){return fetch(url,Object.assign({signal:AbortSignal.timeout?AbortSignal.timeout(5000):undefined,cache:"no-store"},init))}
async function send(field,value){
  changedAt=Date.now();buzz();
  let r;
  try{r=await request("/api/set",{method:"POST",headers:{"X-Aircon":"1"},body:new URLSearchParams({field,value})})}
  catch(e){toast("Couldn't reach the aircon");return}
  if(!r.ok){toast(r.status==400?"The aircon didn't accept that":"Something went wrong ("+r.status+")");poll(true);return}
  apply(await r.json(),true);
}
function set(field,value){if(field=="power"){if(value=="OFF")s.mode="off"}else s[field]=value;render();send(field,value)}
function fill(sel,list){if(sel.options.length)return;for(const v of list){const o=document.createElement("option");o.value=v;o.textContent=v;sel.append(o)}}
function chip(parent,key,label,onclick){const b=document.createElement("button");b.className="chip";b.dataset.v=key;b.textContent=label;b.onclick=onclick;parent.append(b)}
function mark(b,on){b.classList.toggle("on",on);b.setAttribute("aria-pressed",on)}
function render(){
  if(!s)return;
  document.body.dataset.mode=s.mode;
  $("temp").innerHTML=Math.round(s.temperature)+"<small>°"+(s.unit=="Fahrenheit"?"F":"C")+"</small>";
  $("power").setAttribute("aria-pressed",s.mode!="off");
  const m=$("modes");
  if(!m.children.length)for(const k of ["cool","heat","auto","dry","fan_only"])chip(m,k,MODE_LABEL[k],()=>set("mode",k));
  for(const b of m.children)mark(b,b.dataset.v==s.mode);
  for(const k of ["fan_mode","swing_mode","swing_horizontal_mode"]){const e=$(k);fill(e,opts[k]);if(document.activeElement!==e)e.value=s[k]}
  const f=$("features");
  if(!f.children.length)for(const [k,l] of FEATURES)chip(f,k,l,()=>set(k,s[k]=="ON"?"OFF":"ON"));
  for(const b of f.children)mark(b,s[b.dataset.v]=="ON");
  $("learn").classList.toggle("show",!!s.learning);
}
function apply(d,fresh){
  if(lastLearning&&!d.learning&&d.protocol!="Not Set"&&d.protocol!=lastProtocol)toast("Learned "+d.protocol+" 🎉");
  lastLearning=d.learning;lastProtocol=d.protocol;
  if(fresh||!s)s=d;else Object.assign(s,{learning:d.learning,protocol:d.protocol,model:d.model});
  opts=d.options;
  $("name").textContent=d.device.name;document.title=d.device.name;
  const ok=d.device.mqtt;$("dot").className="dot "+(ok?"ok":"bad");
  $("status").textContent=ok?"Home Assistant connected":"Home Assistant offline, local control only";
  $("meta").textContent=(d.protocol=="Not Set"?"No remote learned yet":d.protocol+(d.model>0?" · model "+d.model:""))+" · v"+d.device.version;
  render();
}
async function poll(force){
  if(polling&&!force)return;polling=true;const started=Date.now();
  try{const r=await request("/api/state");const d=await r.json();apply(d,started>changedAt)}
  catch(e){$("dot").className="dot bad";$("status").textContent="Can't reach the aircon"}
  finally{polling=false}
}
function nudge(delta){
  if(!s)return;const t=Math.min(opts.max_temp,Math.max(opts.min_temp,Math.round(s.temperature)+delta));
  if(t==Math.round(s.temperature)){toast(delta>0?"That's as warm as it goes":"That's as cool as it goes");return}
  s.temperature=t;changedAt=Date.now()+1000;buzz();render();
  clearTimeout(tempTimer);tempTimer=setTimeout(()=>send("temperature",String(t)),400);
}
$("up").onclick=()=>nudge(1);$("down").onclick=()=>nudge(-1);
$("power").onclick=()=>s&&set("power",s.mode=="off"?"ON":"OFF");
for(const k of ["fan_mode","swing_mode","swing_horizontal_mode"])$(k).onchange=e=>set(k,e.target.value);
$("learnBtn").onclick=()=>send("learn","PRESS");
$("restart").onclick=()=>{if(confirm("Restart the controller?"))send("restart","PRESS").then(()=>toast("Restarting…"))};
document.addEventListener("visibilitychange",()=>document.hidden||poll());
poll();setInterval(()=>document.hidden||poll(),2500);
</script>
</body>
</html>)rawliteral";
