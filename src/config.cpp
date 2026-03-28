#include "config.h"
#include "looper.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

// Defined in main.cpp
extern void handle_button_press(uint8_t btn);

static PedalConfig cfg;
static DNSServer   dnsServer;
static WebServer   webServer(80);
static Preferences prefs;

static const char* AP_SSID = "TribePedal";
static const char* HOSTNAME = "tribepedal";

// ── HTML ───────────────────────────────────────────────────
static const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Tribe Pedal</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{
  --ink:#0d0b09;--paper:#f4efe6;--brass:#b8923a;--brass-light:#d4aa55;
  --warm:#1e1810;--mid:#2e2620;--cream:#e8e0d0;--muted:#9a8e7e;
}
body{
  font-family:system-ui,-apple-system,sans-serif;font-weight:300;
  background:var(--ink);color:var(--paper);
  min-height:100vh;display:flex;flex-direction:column;align-items:center;
  padding:1.5rem;line-height:1.6;
}
.container{max-width:520px;width:100%;animation:fade-up .8s ease both}
@keyframes fade-up{from{opacity:0;transform:translateY(16px)}to{opacity:1;transform:translateY(0)}}
h1{font-family:Georgia,'Times New Roman',serif;font-size:clamp(1.6rem,4vw,2.2rem);font-weight:400;margin-bottom:.3rem}
.subtitle{font-size:.82rem;color:var(--muted);text-transform:uppercase;letter-spacing:.14em;margin-bottom:.8rem}
.divider{width:48px;height:1px;background:rgba(184,146,58,.5);margin:0 auto 1.5rem}

/* Mode selector */
.modes{display:flex;gap:.5rem;margin-bottom:1.2rem;flex-wrap:wrap}
.modes button{
  flex:1;min-width:0;padding:.55rem .4rem;font-size:.72rem;font-weight:500;
  text-transform:uppercase;letter-spacing:.1em;
  background:transparent;color:var(--muted);
  border:1px solid rgba(184,146,58,.15);border-radius:2px;
  cursor:pointer;transition:all .3s;
}
.modes button:hover{color:var(--brass-light);border-color:var(--brass)}
.modes button.active{background:var(--brass);color:var(--ink);border-color:var(--brass)}

/* LED indicator */
.led-section{text-align:center;margin-bottom:1.4rem}
.led-dot{
  width:32px;height:32px;border-radius:50%;
  display:inline-block;margin:.4rem 0;
  border:1px solid rgba(184,146,58,.2);
  transition:background .3s;
}
.led-state{font-size:.78rem;color:var(--cream);margin-top:.2rem}

/* Button row */
.pedal{display:flex;gap:.5rem;margin-bottom:1.2rem}
.btn-slot{
  flex:1;height:36px;border-radius:3px;
  display:flex;align-items:center;justify-content:center;
  font-size:.7rem;font-weight:500;color:var(--ink);
  cursor:pointer;transition:all .3s;
  border:2px solid transparent;position:relative;
}
.btn-slot.selected{border-color:var(--paper);box-shadow:0 0 8px rgba(184,146,58,.4)}
.btn-label{
  position:absolute;bottom:-1.2rem;left:50%;transform:translateX(-50%);
  font-size:.62rem;color:var(--muted);white-space:nowrap;
}

/* Card */
.card{
  background:var(--warm);border:1px solid rgba(184,146,58,.15);
  border-radius:4px;padding:1.8rem 1.5rem;margin-bottom:1.2rem;
}
.section-label{
  font-size:.72rem;font-weight:500;color:var(--muted);
  text-transform:uppercase;letter-spacing:.14em;margin:1.4rem 0 .5rem;
}
.section-label:first-child{margin-top:0}
label{display:block;font-size:.85rem;color:var(--cream);margin:.5rem 0 .2rem}
select,input[type=number],input[type=text]{
  width:100%;padding:.6rem .7rem;font-size:.9rem;font-weight:300;
  border:1px solid rgba(184,146,58,.15);border-radius:4px;
  background:var(--ink);color:var(--paper);transition:border-color .3s;
  font-family:inherit;
}
select:focus,input:focus{outline:none;border-color:var(--brass)}
.color-row{display:flex;gap:.6rem;margin:.4rem 0}
.color-row label{flex:1;text-align:center;font-size:.75rem}
.color-row input[type=color]{
  width:100%;height:36px;border:1px solid rgba(184,146,58,.15);
  border-radius:4px;cursor:pointer;background:var(--ink);padding:2px;
}
.toggle-row{display:flex;gap:.6rem;margin:.4rem 0}
.toggle-row button{
  flex:1;padding:.5rem;font-size:.78rem;font-weight:500;
  text-transform:uppercase;letter-spacing:.08em;
  background:transparent;color:var(--muted);
  border:1px solid rgba(184,146,58,.15);border-radius:2px;
  cursor:pointer;transition:all .3s;
}
.toggle-row button.active{background:var(--brass);color:var(--ink);border-color:var(--brass)}

/* Conditional fields */
.field-group{display:none}
.field-group.visible{display:block}

/* Save button */
.save-btn{
  width:100%;margin-top:1.2rem;
  background:transparent;color:var(--brass-light);
  border:1px solid var(--brass);border-radius:2px;
  padding:.75rem;font-size:.78rem;font-weight:500;
  text-transform:uppercase;letter-spacing:.14em;
  cursor:pointer;transition:all .3s;
}
.save-btn:hover{background:var(--brass);color:var(--ink)}
.status{margin-top:.6rem;text-align:center;font-size:.82rem;color:var(--brass-light);min-height:1.2em}
.info{font-size:.78rem;color:var(--muted);text-align:center;line-height:1.8}
</style>
</head>
<body>
<div class="container">
  <h1 style="text-align:center">Tribe Pedal</h1>
  <p class="subtitle" style="text-align:center">Configuration</p>
  <div class="divider"></div>

  <p class="section-label" style="text-align:center;margin-bottom:.5rem">Preset</p>
  <select id="mode-sel" onchange="applyMode(this.value)" style="margin-bottom:1.2rem">
    <option value="custom">Custom</option>
    <option value="looper" selected>Looper</option>
    <option value="pageturner">Page Turner</option>
    <option value="pedalboard">Pedalboard</option>
  </select>

  <div class="led-section">
    <div class="led-dot" id="led-dot" style="background:#0000b4"></div>
    <div class="led-state" id="led-state">Clear</div>
  </div>

  <div class="pedal" id="pedal-row"></div>

  <div class="card" id="editor" style="display:none">
    <p class="section-label">Action</p>
    <select id="type" onchange="onTypeChange()">
      <option value="0">MIDI CC</option>
      <option value="1">MIDI Note</option>
      <option value="2">MIDI Program Change</option>
      <option value="3">Keyboard</option>
      <option value="4">OSC</option>
      <option value="5">None</option>
    </select>

    <p class="section-label">Behavior</p>
    <div class="toggle-row">
      <button id="beh-mom" onclick="setBehavior(0)">Momentary</button>
      <button id="beh-tog" onclick="setBehavior(1)">Toggle</button>
    </div>

    <p class="section-label">LED Colors</p>
    <div class="color-row">
      <label>Off / Idle<input type="color" id="col-off" onchange="onColorChange()"></label>
      <label>On / Active<input type="color" id="col-on" onchange="onColorChange()"></label>
    </div>

    <div id="fields-cc" class="field-group">
      <p class="section-label">MIDI CC</p>
      <label>CC Number<input type="number" id="cc-num" min="0" max="127"></label>
      <label>Channel<input type="number" id="cc-ch" min="1" max="16"></label>
      <label>On Value<input type="number" id="cc-on" min="0" max="127"></label>
      <label>Off Value<input type="number" id="cc-off" min="0" max="127"></label>
    </div>

    <div id="fields-note" class="field-group">
      <p class="section-label">MIDI Note</p>
      <label>Note Number<input type="number" id="note-num" min="0" max="127"></label>
      <label>Velocity<input type="number" id="note-vel" min="1" max="127"></label>
      <label>Channel<input type="number" id="note-ch" min="1" max="16"></label>
    </div>

    <div id="fields-pc" class="field-group">
      <p class="section-label">Program Change</p>
      <label>Program<input type="number" id="pc-num" min="0" max="127"></label>
      <label>Channel<input type="number" id="pc-ch" min="1" max="16"></label>
    </div>

    <div id="fields-key" class="field-group">
      <p class="section-label">Keyboard</p>
      <label>Key<input type="text" id="key-char" maxlength="12" placeholder="e.g. a, enter, f5, space"></label>
      <p class="section-label" style="margin-top:.8rem">Modifiers</p>
      <div class="toggle-row">
        <button id="mod-ctrl" onclick="toggleMod(0)">Ctrl</button>
        <button id="mod-shift" onclick="toggleMod(1)">Shift</button>
        <button id="mod-alt" onclick="toggleMod(2)">Alt</button>
        <button id="mod-gui" onclick="toggleMod(3)">Gui</button>
      </div>
    </div>

    <div id="fields-osc" class="field-group">
      <p class="section-label">OSC</p>
      <label>Address<input type="text" id="osc-addr" maxlength="47" placeholder="/looper/1/press"></label>
      <label>On Value<input type="number" id="osc-on" step="0.1"></label>
      <label>Off Value<input type="number" id="osc-off" step="0.1"></label>
    </div>
  </div>

  <button class="save-btn" onclick="saveAll()">Save All</button>
  <div class="status" id="st"></div>

  <div class="card" style="margin-top:1.2rem">
    <p class="section-label" style="margin-top:0">Test Pedal</p>
    <div class="toggle-row" style="margin-top:.6rem" id="test-row"></div>
  </div>

  <p class="info" style="margin-top:1rem">Connect to <strong>TribePedal</strong> WiFi &middot; tribepedal.local</p>
</div>

<script>
var B=[%BTN_JSON%];
var sel=0;

var KEY_MAP={
  'enter':0xB0,'return':0xB0,'esc':0xB1,'escape':0xB1,'backspace':0xB2,
  'tab':0xB3,'space':0x20,'delete':0xD4,'insert':0xD1,
  'up':0xDA,'down':0xD9,'left':0xD8,'right':0xD7,
  'home':0xD2,'end':0xD5,'pageup':0xD3,'pagedown':0xD6,
  'f1':0xC2,'f2':0xC3,'f3':0xC4,'f4':0xC5,'f5':0xC6,'f6':0xC7,
  'f7':0xC8,'f8':0xC9,'f9':0xCA,'f10':0xCB,'f11':0xCC,'f12':0xCD,
  'f13':0xF0,'f14':0xF1,'f15':0xF2,'f16':0xF3,'f17':0xF4,'f18':0xF5,
  'f19':0xF6,'f20':0xF7,'f21':0xF8,'f22':0xF9,'f23':0xFA,'f24':0xFB
};

function rgb2hex(r,g,b){return '#'+[r,g,b].map(function(x){return x.toString(16).padStart(2,'0')}).join('')}
function hex2rgb(h){return[parseInt(h.substr(1,2),16),parseInt(h.substr(3,2),16),parseInt(h.substr(5,2),16)]}

function renderPedal(){
  var row=document.getElementById('pedal-row');
  row.innerHTML='';
  for(var i=0;i<4;i++){
    var b=B[i];
    var d=document.createElement('div');
    d.className='btn-slot'+(i===sel?' selected':'');
    d.style.background=rgb2hex(b.cr,b.cg,b.cb);
    d.innerHTML=(i+1)+'<span class="btn-label">'+['CC','Note','PC','Key','OSC','Off'][b.t]+'</span>';
    d.onclick=(function(idx){return function(){selectBtn(idx)}})(i);
    row.appendChild(d);
  }
}

function selectBtn(i){
  sel=i;
  renderPedal();
  var b=B[i];
  document.getElementById('editor').style.display='block';
  document.getElementById('type').value=b.t;
  document.getElementById('beh-mom').className=b.bh===0?'active':'';
  document.getElementById('beh-tog').className=b.bh===1?'active':'';
  document.getElementById('col-off').value=rgb2hex(b.cr,b.cg,b.cb);
  document.getElementById('col-on').value=rgb2hex(b.cor,b.cog,b.cob);
  document.getElementById('cc-num').value=b.cc;
  document.getElementById('cc-ch').value=b.mch;
  document.getElementById('cc-on').value=b.con;
  document.getElementById('cc-off').value=b.cof;
  document.getElementById('note-num').value=b.nt;
  document.getElementById('note-vel').value=b.vel;
  document.getElementById('note-ch').value=b.mch;
  document.getElementById('pc-num').value=b.pc;
  document.getElementById('pc-ch').value=b.mch;
  var kn=b.kc;
  var ks='';
  for(var k in KEY_MAP){if(KEY_MAP[k]===kn){ks=k;break}}
  if(!ks&&kn>=32&&kn<127)ks=String.fromCharCode(kn);
  document.getElementById('key-char').value=ks;
  document.getElementById('mod-ctrl').className=(b.km&1)?'active':'';
  document.getElementById('mod-shift').className=(b.km&2)?'active':'';
  document.getElementById('mod-alt').className=(b.km&4)?'active':'';
  document.getElementById('mod-gui').className=(b.km&8)?'active':'';
  document.getElementById('osc-addr').value=b.oa;
  document.getElementById('osc-on').value=b.oon;
  document.getElementById('osc-off').value=b.oof;
  onTypeChange();
}

function onTypeChange(){
  var t=parseInt(document.getElementById('type').value);
  B[sel].t=t;
  ['cc','note','pc','key','osc'].forEach(function(f,i){
    document.getElementById('fields-'+f).className='field-group'+(i===t?' visible':'');
  });
  renderPedal();
}

function setBehavior(v){
  B[sel].bh=v;
  document.getElementById('beh-mom').className=v===0?'active':'';
  document.getElementById('beh-tog').className=v===1?'active':'';
}

function onColorChange(){
  var off=hex2rgb(document.getElementById('col-off').value);
  var on=hex2rgb(document.getElementById('col-on').value);
  B[sel].cr=off[0];B[sel].cg=off[1];B[sel].cb=off[2];
  B[sel].cor=on[0];B[sel].cog=on[1];B[sel].cob=on[2];
  renderPedal();
}

function toggleMod(bit){
  B[sel].km^=(1<<bit);
  ['ctrl','shift','alt','gui'].forEach(function(m,i){
    document.getElementById('mod-'+m).className=(B[sel].km&(1<<i))?'active':'';
  });
}

function readFields(){
  var b=B[sel];
  b.t=parseInt(document.getElementById('type').value);
  b.cc=parseInt(document.getElementById('cc-num').value)||0;
  b.mch=parseInt(document.getElementById(b.t===0?'cc-ch':b.t===1?'note-ch':'pc-ch').value)||1;
  b.con=parseInt(document.getElementById('cc-on').value)||127;
  b.cof=parseInt(document.getElementById('cc-off').value)||0;
  b.nt=parseInt(document.getElementById('note-num').value)||60;
  b.vel=parseInt(document.getElementById('note-vel').value)||127;
  b.pc=parseInt(document.getElementById('pc-num').value)||0;
  var ks=document.getElementById('key-char').value.toLowerCase().trim();
  if(KEY_MAP[ks])b.kc=KEY_MAP[ks];
  else if(ks.length===1)b.kc=ks.charCodeAt(0);
  else b.kc=0;
  b.oa=document.getElementById('osc-addr').value;
  b.oon=parseFloat(document.getElementById('osc-on').value)||1;
  b.oof=parseFloat(document.getElementById('osc-off').value)||0;
}

var MODES={
  looper:[
    {t:0,bh:1,cr:0,cg:0,cb:180,cor:255,cog:0,cob:0,kc:0,km:0,cc:37,mch:11,con:127,cof:0,nt:60,vel:127,pc:0,oa:'/sl/0/hit',oon:1,oof:0},
    {t:0,bh:1,cr:128,cg:0,cb:180,cor:0,cog:255,cob:0,kc:0,km:0,cc:38,mch:11,con:127,cof:0,nt:60,vel:127,pc:0,oa:'/sl/0/hit',oon:1,oof:0},
    {t:0,bh:0,cr:0,cg:0,cb:180,cor:255,cog:255,cob:255,kc:0,km:0,cc:39,mch:11,con:127,cof:0,nt:60,vel:127,pc:0,oa:'/sl/0/hit',oon:1,oof:0},
    {t:0,bh:0,cr:0,cg:0,cb:180,cor:255,cog:255,cob:255,kc:0,km:0,cc:40,mch:11,con:127,cof:0,nt:60,vel:127,pc:0,oa:'/sl/0/hit',oon:1,oof:0}
  ],
  pageturner:[
    {t:3,bh:0,cr:30,cg:30,cb:50,cor:200,cog:200,cob:255,kc:0xD8,km:0,cc:1,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:3,bh:0,cr:30,cg:30,cb:50,cor:200,cog:200,cob:255,kc:0xD7,km:0,cc:1,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:3,bh:0,cr:30,cg:30,cb:50,cor:200,cog:200,cob:255,kc:0xDA,km:0,cc:1,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:3,bh:0,cr:30,cg:30,cb:50,cor:200,cog:200,cob:255,kc:0xD9,km:0,cc:1,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0}
  ],
  pedalboard:[
    {t:0,bh:1,cr:20,cg:40,cb:10,cor:0,cog:255,cob:0,kc:0,km:0,cc:80,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:0,bh:1,cr:40,cg:20,cb:10,cor:255,cog:140,cob:0,kc:0,km:0,cc:81,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:0,bh:1,cr:10,cg:20,cb:40,cor:80,cog:80,cob:255,kc:0,km:0,cc:82,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:0,bh:1,cr:40,cg:10,cb:10,cor:255,cog:0,cob:0,kc:0,km:0,cc:83,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0}
  ]
};

var TEST_LABELS={
  custom:['1','2','3','4'],
  looper:['Rec','Play','Undo','Reset'],
  pageturner:['Left','Right','Up','Down'],
  pedalboard:['FX 1','FX 2','FX 3','FX 4']
};
var currentMode='looper';

function renderTestButtons(){
  var labels=TEST_LABELS[currentMode]||TEST_LABELS.custom;
  var row=document.getElementById('test-row');
  row.innerHTML='';
  for(var i=0;i<4;i++){
    var b=document.createElement('button');
    b.textContent=labels[i];
    b.style.color='var(--paper)';
    b.onclick=(function(idx){return function(){testPress(idx)}})(i);
    row.appendChild(b);
  }
}

function applyMode(m){
  currentMode=m;
  if(MODES[m]){
    B=JSON.parse(JSON.stringify(MODES[m]));
    selectBtn(0);
  }
  renderTestButtons();
  updateLed(0);
  fetch('/press?b=99');
}

async function saveAll(){
  readFields();
  var res=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(B)});
  document.getElementById('st').textContent=res.ok?'Saved!':'Error';
  if(res.ok)setTimeout(function(){document.getElementById('st').textContent=''},2000);
}

var STATE_NAMES=['Clear','Recording','Playing','Overdubbing','Paused'];
var STATE_COLORS=['#0000b4','#ff0000','#00ff00','#ff0000','#8000b4'];

function updateLed(s){
  document.getElementById('led-dot').style.background=STATE_COLORS[s];
  document.getElementById('led-state').textContent=STATE_NAMES[s];
}

async function testPress(btn){
  // Flash white briefly
  document.getElementById('led-dot').style.background='#ffffff';
  var res=await fetch('/press?b='+btn);
  if(res.ok){
    var j=await res.json();
    setTimeout(function(){updateLed(j.s)},120);
  }
}

renderPedal();
selectBtn(0);
renderTestButtons();
</script>
</body>
</html>
)rawliteral";

// ── JSON helpers ───────────────────────────────────────────
static String buttonToJson(const ButtonConfig& b, int idx) {
    char buf[320];
    snprintf(buf, sizeof(buf),
        "{\"t\":%d,\"bh\":%d,"
        "\"cr\":%d,\"cg\":%d,\"cb\":%d,"
        "\"cor\":%d,\"cog\":%d,\"cob\":%d,"
        "\"mch\":%d,\"cc\":%d,\"con\":%d,\"cof\":%d,"
        "\"nt\":%d,\"vel\":%d,\"pc\":%d,"
        "\"km\":%d,\"kc\":%d,"
        "\"oa\":\"%s\",\"oon\":%.1f,\"oof\":%.1f}",
        b.type, b.behavior,
        b.color_r, b.color_g, b.color_b,
        b.color_on_r, b.color_on_g, b.color_on_b,
        b.midi_channel, b.midi_cc, b.midi_cc_on, b.midi_cc_off,
        b.midi_note, b.midi_velocity, b.midi_program,
        b.key_modifiers, b.key_code,
        b.osc_addr, b.osc_on, b.osc_off
    );
    return String(buf);
}

static String allButtonsJson() {
    String json = "";
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (i > 0) json += ",";
        json += buttonToJson(cfg.buttons[i], i);
    }
    return json;
}

static String buildPage() {
    String page = HTML_PAGE;
    page.replace("%BTN_JSON%", allButtonsJson());
    return page;
}

// ── Route handlers ─────────────────────────────────────────
static void handleRoot() {
    webServer.send(200, "text/html", buildPage());
}

static void handleSave() {
    String body = webServer.arg("plain");

    // Minimal JSON array parsing — we know the structure
    int idx = 0;
    int pos = 0;
    while (idx < NUM_BUTTONS && pos < (int)body.length()) {
        int objStart = body.indexOf('{', pos);
        if (objStart < 0) break;
        int objEnd = body.indexOf('}', objStart);
        if (objEnd < 0) break;
        String obj = body.substring(objStart, objEnd + 1);

        ButtonConfig& b = cfg.buttons[idx];

        // Parse each known field
        auto getInt = [&](const char* key) -> int {
            String k = String("\"") + key + "\":";
            int p = obj.indexOf(k);
            if (p < 0) return 0;
            return obj.substring(p + k.length()).toInt();
        };
        auto getFloat = [&](const char* key) -> float {
            String k = String("\"") + key + "\":";
            int p = obj.indexOf(k);
            if (p < 0) return 0.0f;
            return obj.substring(p + k.length()).toFloat();
        };
        auto getStr = [&](const char* key, char* dst, int maxLen) {
            String k = String("\"") + key + "\":\"";
            int p = obj.indexOf(k);
            if (p < 0) { dst[0] = 0; return; }
            int start = p + k.length();
            int end = obj.indexOf('"', start);
            if (end < 0) { dst[0] = 0; return; }
            String val = obj.substring(start, end);
            strncpy(dst, val.c_str(), maxLen - 1);
            dst[maxLen - 1] = 0;
        };

        b.type         = (ActionType)getInt("t");
        b.behavior     = (ButtonBehavior)getInt("bh");
        b.color_r      = getInt("cr");
        b.color_g      = getInt("cg");
        b.color_b      = getInt("cb");
        b.color_on_r   = getInt("cor");
        b.color_on_g   = getInt("cog");
        b.color_on_b   = getInt("cob");
        b.midi_channel  = getInt("mch");
        b.midi_cc       = getInt("cc");
        b.midi_cc_on    = getInt("con");
        b.midi_cc_off   = getInt("cof");
        b.midi_note     = getInt("nt");
        b.midi_velocity = getInt("vel");
        b.midi_program  = getInt("pc");
        b.key_modifiers = getInt("km");
        b.key_code      = getInt("kc");
        b.osc_on        = getFloat("oon");
        b.osc_off       = getFloat("oof");
        getStr("oa", b.osc_addr, sizeof(b.osc_addr));

        idx++;
        pos = objEnd + 1;
    }

    config_save();
    webServer.send(200, "text/plain", "OK");
}

static void handlePress() {
    if (webServer.hasArg("b")) {
        int btn = webServer.arg("b").toInt();
        if (btn == 99) {
            // Reset looper state (mode change)
            looper_init();
            webServer.send(200, "application/json", "{\"s\":0}");
            return;
        }
        if (btn >= 0 && btn < NUM_BUTTONS) {
            handle_button_press((uint8_t)btn);
            String json = "{\"s\":" + String(looper_state()) + "}";
            webServer.send(200, "application/json", json);
            return;
        }
    }
    webServer.send(400, "text/plain", "Bad request");
}

static void handleNotFound() {
    webServer.sendHeader("Location", "http://tribepedal.local/", true);
    webServer.send(302, "text/plain", "");
}

// ── NVS ────────────────────────────────────────────────────
// Looper defaults per button (used when NVS is empty)
struct LooperDefault {
    uint8_t behavior;
    uint8_t cr, cg, cb;        // idle color
    uint8_t cor, cog, cob;     // active color
    uint8_t cc;
};
static const LooperDefault LOOPER_DEFAULTS[NUM_BUTTONS] = {
    { BEHAVIOR_TOGGLE,    0, 0, 180,   255, 0,   0,    37 },  // Record/Overdub: blue → red
    { BEHAVIOR_TOGGLE,    128, 0, 180,  0,   255, 0,    38 },  // Pause/Play: purple → green
    { BEHAVIOR_MOMENTARY, 0, 0, 180,   255, 255, 255,  39 },  // Undo: blue → white flash
    { BEHAVIOR_MOMENTARY, 0, 0, 180,   255, 255, 255,  40 },  // Reset: blue → white flash
};

static void loadButton(int i) {
    char prefix[8];
    snprintf(prefix, sizeof(prefix), "b%d_", i);
    ButtonConfig& b = cfg.buttons[i];
    const LooperDefault& d = LOOPER_DEFAULTS[i];

    auto key = [&](const char* suffix) -> String { return String(prefix) + suffix; };

    b.type         = (ActionType)prefs.getUChar(key("t").c_str(), ACTION_MIDI_CC);
    b.behavior     = (ButtonBehavior)prefs.getUChar(key("bh").c_str(), d.behavior);
    b.color_r      = prefs.getUChar(key("cr").c_str(), d.cr);
    b.color_g      = prefs.getUChar(key("cg").c_str(), d.cg);
    b.color_b      = prefs.getUChar(key("cb").c_str(), d.cb);
    b.color_on_r   = prefs.getUChar(key("cor").c_str(), d.cor);
    b.color_on_g   = prefs.getUChar(key("cog").c_str(), d.cog);
    b.color_on_b   = prefs.getUChar(key("cob").c_str(), d.cob);
    b.midi_channel  = prefs.getUChar(key("mch").c_str(), 11);
    b.midi_cc       = prefs.getUChar(key("cc").c_str(), d.cc);
    b.midi_cc_on    = prefs.getUChar(key("con").c_str(), 127);
    b.midi_cc_off   = prefs.getUChar(key("cof").c_str(), 0);
    b.midi_note     = prefs.getUChar(key("nt").c_str(), 60 + i);
    b.midi_velocity = prefs.getUChar(key("vel").c_str(), 127);
    b.midi_program  = prefs.getUChar(key("pc").c_str(), i);
    b.key_modifiers = prefs.getUChar(key("km").c_str(), 0);
    b.key_code      = prefs.getUChar(key("kc").c_str(), 0);
    b.osc_on        = prefs.getFloat(key("oon").c_str(), 1.0f);
    b.osc_off       = prefs.getFloat(key("oof").c_str(), 0.0f);

    String oa = prefs.getString(key("oa").c_str(), String("/sl/0/hit"));
    strncpy(b.osc_addr, oa.c_str(), sizeof(b.osc_addr) - 1);
}

static void saveButton(int i) {
    char prefix[8];
    snprintf(prefix, sizeof(prefix), "b%d_", i);
    const ButtonConfig& b = cfg.buttons[i];

    auto key = [&](const char* suffix) -> String { return String(prefix) + suffix; };

    prefs.putUChar(key("t").c_str(), b.type);
    prefs.putUChar(key("bh").c_str(), b.behavior);
    prefs.putUChar(key("cr").c_str(), b.color_r);
    prefs.putUChar(key("cg").c_str(), b.color_g);
    prefs.putUChar(key("cb").c_str(), b.color_b);
    prefs.putUChar(key("cor").c_str(), b.color_on_r);
    prefs.putUChar(key("cog").c_str(), b.color_on_g);
    prefs.putUChar(key("cob").c_str(), b.color_on_b);
    prefs.putUChar(key("mch").c_str(), b.midi_channel);
    prefs.putUChar(key("cc").c_str(), b.midi_cc);
    prefs.putUChar(key("con").c_str(), b.midi_cc_on);
    prefs.putUChar(key("cof").c_str(), b.midi_cc_off);
    prefs.putUChar(key("nt").c_str(), b.midi_note);
    prefs.putUChar(key("vel").c_str(), b.midi_velocity);
    prefs.putUChar(key("pc").c_str(), b.midi_program);
    prefs.putUChar(key("km").c_str(), b.key_modifiers);
    prefs.putUChar(key("kc").c_str(), b.key_code);
    prefs.putFloat(key("oon").c_str(), b.osc_on);
    prefs.putFloat(key("oof").c_str(), b.osc_off);
    prefs.putString(key("oa").c_str(), b.osc_addr);
}

// ── Public API ─────────────────────────────────────────────
void config_init() {
    prefs.begin("pedal", true);
    cfg.led_brightness = prefs.getUChar("bright", 30);
    for (int i = 0; i < NUM_BUTTONS; i++) loadButton(i);
    prefs.end();

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    dnsServer.start(53, "*", WiFi.softAPIP());
    MDNS.begin(HOSTNAME);

    webServer.on("/", handleRoot);
    webServer.on("/save", HTTP_POST, handleSave);
    webServer.on("/press", handlePress);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
}

void config_loop() {
    dnsServer.processNextRequest();
    webServer.handleClient();
}

PedalConfig& config_get() {
    return cfg;
}

void config_save() {
    prefs.begin("pedal", false);
    prefs.putUChar("bright", cfg.led_brightness);
    for (int i = 0; i < NUM_BUTTONS; i++) saveButton(i);
    prefs.end();
}
