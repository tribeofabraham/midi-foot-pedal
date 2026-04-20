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

static char pedal_name[33] = "TribePedal";

// ── HTML (synced from ui-preview.html) ─────────────────────
static const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
<title>Tribe Pedal</title>
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="Tribe Pedal">
<meta name="mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#111111">
<link rel="icon" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAAsTAAALEwEAmpwYAAAFeklEQVR4nO1Wa0yTVxh+e4e2tNwEK/TCRWih1EKho1Boy1UpgaICCgIqqEO8TAE3jDpk4ma8TVA2Lz+mIiPo5jbnDLuoi8ncry1bsriYTbP9WJZlbgnZAijyLKerIJty2eqfZU/yJt93vvc7z/Od9znvd4j+x/QRQ0SSaeRJichB3oY8JvcnaYT1NhEpJkmbk2ANuWUv1YwSkcirAoISK+5GLesFE0JEUY9I0Vmcyjvbe2woWhMLIgr2Jj9/lrkW6uIOqEsOIyBh4QARGcefkjW3Mmqw/kAqandbsGhDHBOg8qaAwND09TDYnUjOykV01VkEmWqGiPg2oS9voatBO7Kj146sVXakLS/EkuYEJkDnTQERCseziK7qQ/iCdsy2NSF6WR9C0hpGIvRBgy0nM5BXHY3yJj2YkOrtRiYg2ZsCjGF5OxE+vx3E4bDJIVWnIqqiBwr7FvhIfd1jLJZsSUDtriR2bf83hGLmOyKKJCI9EdUEGZfCL8I6RsTCN0SH8Pm7wBP5jY1lV0Sifl8Ku97q2Y5GzzyBbrdMgsTIopW/61c9D8Pa3TA1d8K87QRS27qRse9t2Dv74TjyITg83gQRfw2rS4WNh1Oxf6MPvjwjxbWjEry7X4KeNjGOPueLjeXCQSIyP0qAVJFWcNt57ibsHZdgXFaDwje/gaXtDJio2KWbEFlcBwGXB6VQCIGnFCxEHA7CPWNJ2Qps6rKg0MrH+T1i4FM5XEVx+LzbD0PX5HCm829N1tDCNQuqfrEf/gDGFevwVOvJCV9nk8tw2uHAxTgd3pkbDbNEgmyZH96aG41+bSxeS0+H2W+8HCzYCqyo0OFGrwyV+QLWQ8Kmqr8+tmLzYOH5b2FY2z42UQCPhx67DRe0sfjYE1c88eD+YmwMujMzIOFyx95jy89WYX2ZcMDTzqcFm6HhxZGCvhsICQtGoIyDbJkMl3TaMbLHxQV9PNKkUjd5dYEQo9flaK0TDRGRiWYCro9PmdKSfe+T41L3V+TJZVOSPwgmwGHiY/iaHF1bfEeIKJtmCrmUXBcPSEbvvC+DTsNFmFCIq9MgvxwbgwA+H5ePSPD6C2IoLPmDPJ4of0bkfD7ZuneKR5hrM4z8sXpun6OYUsDm2aHuXIkvB7I5SjjfuAl93Y7hmZTA0NnoM8SMw/btw45m5jqh0TyW/BW1Gj4PGZBFUuMhMENHLaof8DSkSRHxfJ1o4LNTMiwqScCVLgk4nImNRsbjoVOl+hv5QZVygvtZcDhcpO85B2PNaji6PoIyu/SHyX7TgetKhT8y137RLUNNuda9fa6fkKLELkDTsXR3k2ET8zkcbAoNdRMzX9SHzHKPcQVCRLlWQVvVjIQ1bW5y1sxYU7N19KPg7NcITcn56nGHFfPTJcI7rXWiu/s2+OBYiy/62sXoPyTBhjIhnumyIK1INeELU6USmCTisXu+WIqsV6/C9vJ7sO49j9TWU0hpOYqkpg4Y6tsRv3IbVPkVbEsmTlUK4UM/I2aexoYDZjjKI8bJJEEIy2+DKFAzQZQ8Jg8B8S52Xe15N84zTwAR+dA/RObql5JRujl+3JD+MoTltSJyySmIFYY/a87lQ1V0ELMzGzEdw80EpuWtie7DRlmjHs7aGGw9nQm1PnhYYWsejarsdZMqnXs9B5W1Xj8TaitbDEgsdcJRZ3cLKaiLGeEKuGXEFZSHWOrvs2Naeo4VcbZiBCfXMAECbwpQsuWvbU/F+kMWZC7WsH961vhjXm6QqWpYXdKFiMXHETiv/B55GYGuBh22nbHBlKP4lYjmPSInxT+++Dd2bvSPd7Gm41XwbYs197Xm4O+JSD1JnlYWnfOzNDLjO3oCKCAi/2nkhbB2/iQE0H8SfwDB5dfWAE9+FAAAAABJRU5ErkJggg==" type="image/png">
<link rel="apple-touch-icon" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAAsTAAALEwEAmpwYAAAFeklEQVR4nO1Wa0yTVxh+e4e2tNwEK/TCRWih1EKho1Boy1UpgaICCgIqqEO8TAE3jDpk4ma8TVA2Lz+mIiPo5jbnDLuoi8ncry1bsriYTbP9WJZlbgnZAijyLKerIJty2eqfZU/yJt93vvc7z/Od9znvd4j+x/QRQ0SSaeRJichB3oY8JvcnaYT1NhEpJkmbk2ANuWUv1YwSkcirAoISK+5GLesFE0JEUY9I0Vmcyjvbe2woWhMLIgr2Jj9/lrkW6uIOqEsOIyBh4QARGcefkjW3Mmqw/kAqandbsGhDHBOg8qaAwND09TDYnUjOykV01VkEmWqGiPg2oS9voatBO7Kj146sVXakLS/EkuYEJkDnTQERCseziK7qQ/iCdsy2NSF6WR9C0hpGIvRBgy0nM5BXHY3yJj2YkOrtRiYg2ZsCjGF5OxE+vx3E4bDJIVWnIqqiBwr7FvhIfd1jLJZsSUDtriR2bf83hGLmOyKKJCI9EdUEGZfCL8I6RsTCN0SH8Pm7wBP5jY1lV0Sifl8Ku97q2Y5GzzyBbrdMgsTIopW/61c9D8Pa3TA1d8K87QRS27qRse9t2Dv74TjyITg83gQRfw2rS4WNh1Oxf6MPvjwjxbWjEry7X4KeNjGOPueLjeXCQSIyP0qAVJFWcNt57ibsHZdgXFaDwje/gaXtDJio2KWbEFlcBwGXB6VQCIGnFCxEHA7CPWNJ2Qps6rKg0MrH+T1i4FM5XEVx+LzbD0PX5HCm829N1tDCNQuqfrEf/gDGFevwVOvJCV9nk8tw2uHAxTgd3pkbDbNEgmyZH96aG41+bSxeS0+H2W+8HCzYCqyo0OFGrwyV+QLWQ8Kmqr8+tmLzYOH5b2FY2z42UQCPhx67DRe0sfjYE1c88eD+YmwMujMzIOFyx95jy89WYX2ZcMDTzqcFm6HhxZGCvhsICQtGoIyDbJkMl3TaMbLHxQV9PNKkUjd5dYEQo9flaK0TDRGRiWYCro9PmdKSfe+T41L3V+TJZVOSPwgmwGHiY/iaHF1bfEeIKJtmCrmUXBcPSEbvvC+DTsNFmFCIq9MgvxwbgwA+H5ePSPD6C2IoLPmDPJ4of0bkfD7ZuneKR5hrM4z8sXpun6OYUsDm2aHuXIkvB7I5SjjfuAl93Y7hmZTA0NnoM8SMw/btw45m5jqh0TyW/BW1Gj4PGZBFUuMhMENHLaof8DSkSRHxfJ1o4LNTMiwqScCVLgk4nImNRsbjoVOl+hv5QZVygvtZcDhcpO85B2PNaji6PoIyu/SHyX7TgetKhT8y137RLUNNuda9fa6fkKLELkDTsXR3k2ET8zkcbAoNdRMzX9SHzHKPcQVCRLlWQVvVjIQ1bW5y1sxYU7N19KPg7NcITcn56nGHFfPTJcI7rXWiu/s2+OBYiy/62sXoPyTBhjIhnumyIK1INeELU6USmCTisXu+WIqsV6/C9vJ7sO49j9TWU0hpOYqkpg4Y6tsRv3IbVPkVbEsmTlUK4UM/I2aexoYDZjjKI8bJJEEIy2+DKFAzQZQ8Jg8B8S52Xe15N84zTwAR+dA/RObql5JRujl+3JD+MoTltSJyySmIFYY/a87lQ1V0ELMzGzEdw80EpuWtie7DRlmjHs7aGGw9nQm1PnhYYWsejarsdZMqnXs9B5W1Xj8TaitbDEgsdcJRZ3cLKaiLGeEKuGXEFZSHWOrvs2Naeo4VcbZiBCfXMAECbwpQsuWvbU/F+kMWZC7WsH961vhjXm6QqWpYXdKFiMXHETiv/B55GYGuBh22nbHBlKP4lYjmPSInxT+++Dd2bvSPd7Gm41XwbYs197Xm4O+JSD1JnlYWnfOzNDLjO3oCKCAi/2nkhbB2/iQE0H8SfwDB5dfWAE9+FAAAAABJRU5ErkJggg==">
<link rel="manifest" href="/manifest.webmanifest">
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{
  --ink:#111111;--paper:#f0f0f0;--accent:#4a90d9;--accent-light:#6aabef;
  --warm:#1a1a1a;--mid:#2a2a2a;--cream:#d8d8d8;--muted:#999999;
}
body{
  font-family:system-ui,-apple-system,sans-serif;font-weight:300;
  background:var(--ink);color:var(--paper);
  min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center;
  padding:1.5rem;padding-top:env(safe-area-inset-top,1.5rem);line-height:1.6;
}
.container{max-width:520px;width:100%;animation:fade-up .8s ease both;position:relative}
@keyframes fade-up{from{opacity:0;transform:translateY(16px)}to{opacity:1;transform:translateY(0)}}
@media(prefers-reduced-motion:reduce){.container{animation:none}}

/* Toolbar: buttons + preset on one line */
.toolbar{display:flex;align-items:center;gap:.8rem;justify-content:center;margin-bottom:2rem}
.preset-sel{
  padding:.45rem .5rem;font-size:.78rem;font-weight:400;
  border:1px solid rgba(74,144,217,.3);border-radius:4px;
  background:var(--ink);color:var(--paper);font-family:inherit;
  width:auto;
}
.preset-sel:focus{outline:2px solid var(--accent-light);outline-offset:1px;border-color:var(--accent)}

/* Button selector row */
.pedal{display:flex;gap:.8rem}
.btn-slot{
  width:48px;height:48px;border-radius:50%;
  display:flex;align-items:center;justify-content:center;
  font-size:.75rem;font-weight:600;
  cursor:pointer;transition:all .2s;
  border:3px solid transparent;
  position:relative;
}
.btn-slot:hover{transform:scale(1.08)}
.btn-slot:focus-visible{outline:3px solid var(--accent-light);outline-offset:3px}
.btn-slot[aria-selected="true"]{border-color:var(--paper);box-shadow:0 0 12px rgba(74,144,217,.5)}
.btn-label{
  position:absolute;top:calc(100% + .4rem);left:50%;transform:translateX(-50%);
  font-size:.65rem;font-weight:400;color:var(--cream);white-space:nowrap;
}

/* Card */
.card{
  background:var(--warm);border:1px solid rgba(74,144,217,.15);
  border-radius:12px;padding:1.8rem 1.5rem;margin-bottom:1.2rem;
}
.section-label{
  font-size:.72rem;font-weight:500;color:var(--cream);
  text-transform:uppercase;letter-spacing:.14em;margin:1.4rem 0 .5rem;
}
.section-label:first-child{margin-top:0}

/* Form elements */
label{display:flex;align-items:center;gap:.6rem;font-size:.85rem;color:var(--cream);margin:.4rem 0}
label>input,label>select{flex:1;min-width:0}
select,input[type=number],input[type=text]{
  width:100%;padding:.65rem .75rem;font-size:.9rem;font-weight:300;
  border:1px solid rgba(74,144,217,.3);border-radius:4px;
  background:var(--ink);color:var(--paper);transition:border-color .3s;
  font-family:inherit;
}
select:focus,input:focus{outline:2px solid var(--accent-light);outline-offset:1px;border-color:var(--accent)}
select{color:var(--paper);appearance:auto}
.color-row{display:flex;gap:.8rem;margin:.4rem 0}
.color-row label{flex:1;text-align:center;font-size:.78rem}
.color-row input[type=color]{
  width:100%;height:40px;border:1px solid rgba(74,144,217,.3);
  border-radius:4px;cursor:pointer;background:var(--ink);padding:2px;
}
.color-row input[type=color]:focus{outline:2px solid var(--accent-light);outline-offset:1px}

/* Toggle buttons */
.toggle-row{display:flex;gap:.6rem;margin:.4rem 0}
.toggle-row button{
  flex:1;padding:.55rem;font-size:.78rem;font-weight:500;
  text-transform:uppercase;letter-spacing:.08em;
  background:transparent;color:var(--cream);
  border:1px solid rgba(74,144,217,.3);border-radius:2px;
  cursor:pointer;transition:all .2s;
}
.toggle-row button:hover{border-color:var(--accent);color:var(--accent-light)}
.toggle-row button:focus-visible{outline:2px solid var(--accent-light);outline-offset:2px}
.toggle-row button[aria-pressed="true"]{background:var(--accent);color:var(--ink);border-color:var(--accent)}

/* Conditional fields */
.field-group{display:none}
.field-group.visible{display:block}

/* Save button */
.save-btn{
  width:100%;margin-top:1.2rem;
  background:transparent;color:var(--accent-light);
  border:1px solid var(--accent);border-radius:2px;
  padding:.8rem;font-size:.82rem;font-weight:500;
  text-transform:uppercase;letter-spacing:.14em;
  cursor:pointer;transition:all .2s;
}
.save-btn:hover{background:var(--accent);color:var(--ink)}
.save-btn:focus-visible{outline:2px solid var(--accent-light);outline-offset:3px}
.status{margin-top:.6rem;text-align:center;font-size:.85rem;color:var(--accent-light);min-height:1.2em}
[role="status"]{min-height:1.2em}
.info{font-size:.8rem;color:var(--cream);text-align:center;line-height:1.8;margin-top:1.5rem}

/* Test section */
.test-row{display:flex;gap:.6rem;margin-top:.6rem}
.test-row button{
  flex:1;padding:.55rem;font-size:.78rem;font-weight:500;
  text-transform:uppercase;letter-spacing:.08em;
  background:transparent;color:var(--cream);
  border:1px solid rgba(74,144,217,.3);border-radius:2px;
  cursor:pointer;transition:all .2s;
}
.test-row button:hover{border-color:var(--accent);color:var(--accent-light)}
.test-row button:focus-visible{outline:2px solid var(--accent-light);outline-offset:2px}
</style>
</head>
<body>
<div class="container" role="main">

  <div class="toolbar" role="toolbar" aria-label="Button selector and preset">
    <div class="pedal" id="pedal-row" role="tablist"></div>
    <select id="mode-sel" onchange="applyMode(this.value)" aria-label="Preset" class="preset-sel">
      <option value="custom">Custom</option>
      <option value="looper" selected>Looper</option>
      <option value="zlooper">Z-Looper</option>
      <option value="pageturner">Page Turner</option>
      <option value="pedalboard">Pedalboard</option>
    </select>
  </div>

  <div class="card" id="editor" style="display:none" role="tabpanel" aria-label="Button configuration">
    <label class="section-label" style="margin-top:0">Action <select id="type" onchange="onTypeChange()" style="flex:1">
      <option value="0">MIDI CC</option>
      <option value="1">MIDI Note</option>
      <option value="2">MIDI Program Change</option>
      <option value="3">Keyboard</option>
      <option value="4">OSC</option>
      <option value="5">None</option>
    </select></label>

    <p class="section-label" id="behavior-label">Behavior</p>
    <div class="toggle-row" role="group" aria-labelledby="behavior-label">
      <button type="button" id="beh-mom" onclick="setBehavior(0)" aria-pressed="true">Momentary</button>
      <button type="button" id="beh-tog" onclick="setBehavior(1)" aria-pressed="false">Toggle</button>
    </div>

    <p class="section-label" id="color-label">LED Colors</p>
    <div class="color-row" role="group" aria-labelledby="color-label">
      <label>Off / Idle<input type="color" id="col-off" onchange="onColorChange()" aria-label="LED color when off"></label>
      <label>On / Active<input type="color" id="col-on" onchange="onColorChange()" aria-label="LED color when on"></label>
    </div>

    <div id="fields-cc" class="field-group">
      <fieldset style="border:0;padding:0;margin:0">
        <legend class="section-label">MIDI CC</legend>
        <label>CC Number <input type="number" id="cc-num" min="0" max="127"></label>
        <label>Channel <input type="number" id="cc-ch" min="1" max="16"></label>
        <label>On Value <input type="number" id="cc-on" min="0" max="127"></label>
        <label>Off Value <input type="number" id="cc-off" min="0" max="127"></label>
      </fieldset>
    </div>

    <div id="fields-note" class="field-group">
      <fieldset style="border:0;padding:0;margin:0">
        <legend class="section-label">MIDI Note</legend>
        <label>Note Number <input type="number" id="note-num" min="0" max="127"></label>
        <label>Velocity <input type="number" id="note-vel" min="1" max="127"></label>
        <label>Channel <input type="number" id="note-ch" min="1" max="16"></label>
      </fieldset>
    </div>

    <div id="fields-pc" class="field-group">
      <fieldset style="border:0;padding:0;margin:0">
        <legend class="section-label">Program Change</legend>
        <label>Program <input type="number" id="pc-num" min="0" max="127"></label>
        <label>Channel <input type="number" id="pc-ch" min="1" max="16"></label>
      </fieldset>
    </div>

    <div id="fields-key" class="field-group">
      <fieldset style="border:0;padding:0;margin:0">
        <legend class="section-label">Keyboard</legend>
        <label>Key <input type="text" id="key-char" maxlength="12" placeholder="e.g. a, enter, f5, space"></label>
        <p class="section-label" style="margin-top:.8rem" id="mod-label">Modifiers</p>
        <div class="toggle-row" role="group" aria-labelledby="mod-label">
          <button type="button" id="mod-ctrl" onclick="toggleMod(0)" aria-pressed="false">Ctrl</button>
          <button type="button" id="mod-shift" onclick="toggleMod(1)" aria-pressed="false">Shift</button>
          <button type="button" id="mod-alt" onclick="toggleMod(2)" aria-pressed="false">Alt</button>
          <button type="button" id="mod-gui" onclick="toggleMod(3)" aria-pressed="false">Gui</button>
        </div>
      </fieldset>
    </div>

    <div id="fields-osc" class="field-group">
      <fieldset style="border:0;padding:0;margin:0">
        <legend class="section-label">OSC</legend>
        <label>Address <input type="text" id="osc-addr" maxlength="47" placeholder="/looper/1/press"></label>
        <label>On Value <input type="number" id="osc-on" step="0.1"></label>
        <label>Off Value <input type="number" id="osc-off" step="0.1"></label>
      </fieldset>
    </div>
  </div>

  <button type="button" class="save-btn" onclick="saveAll()">Save All</button>
  <div class="status" id="st" role="status" aria-live="polite"></div>

  <div class="card" style="margin-top:1.2rem">
    <p class="section-label" style="margin-top:0" id="test-label">Test Pedal</p>
    <div class="test-row" id="test-row" role="group" aria-labelledby="test-label"></div>
  </div>

  <div class="card" style="margin-top:1.2rem">
    <p class="section-label" style="margin-top:0">Pedal Name</p>
    <input type="text" id="pedal-name" value="%PEDAL_NAME%" maxlength="32" style="width:100%;box-sizing:border-box;padding:0.4rem;font-size:1rem;border-radius:6px;border:1px solid #444;background:#222;color:#eee" aria-label="Pedal name (WiFi SSID)">
    <p class="info" style="margin-top:0.4rem">WiFi network name &amp; <span id="mdns-hint">%PEDAL_NAME_LC%.local</span></p>
  </div>
</div>

<script>
var B=[%BTN_JSON%];
var sel=0;
var initMode=%MODE%;

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

var BTN_NAMES=['Button 1','Button 2','Button 3','Button 4'];

function rgb2hex(r,g,b){return '#'+[r,g,b].map(function(x){return x.toString(16).padStart(2,'0')}).join('')}
function hex2rgb(h){return[parseInt(h.substr(1,2),16),parseInt(h.substr(3,2),16),parseInt(h.substr(5,2),16)]}

// Ensure text on colored backgrounds meets WCAG AA contrast
function contrastColor(r,g,b){
  var lum=(0.299*r+0.587*g+0.114*b)/255;
  return lum>0.5?'#0d0b09':'#f4efe6';
}

function renderPedal(){
  var row=document.getElementById('pedal-row');
  row.innerHTML='';
  var labels=['CC','Note','PC','Key','OSC','Off'];
  for(var i=0;i<4;i++){
    var b=B[i];
    var bg=rgb2hex(b.cr,b.cg,b.cb);
    var fg=contrastColor(b.cr,b.cg,b.cb);
    var d=document.createElement('button');
    d.type='button';
    d.className='btn-slot';
    d.setAttribute('role','tab');
    d.setAttribute('aria-selected',i===sel?'true':'false');
    d.setAttribute('aria-label',BTN_NAMES[i]+' — '+labels[b.t]);
    d.style.background=bg;
    d.style.color=fg;
    d.innerHTML=(i+1)+'<span class="btn-label" aria-hidden="true">'+labels[b.t]+'</span>';
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
  document.getElementById('beh-mom').setAttribute('aria-pressed',b.bh===0?'true':'false');
  document.getElementById('beh-tog').setAttribute('aria-pressed',b.bh===1?'true':'false');
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
  ['ctrl','shift','alt','gui'].forEach(function(m,idx){
    document.getElementById('mod-'+m).setAttribute('aria-pressed',(b.km&(1<<idx))?'true':'false');
  });
  document.getElementById('osc-addr').value=b.oa;
  document.getElementById('osc-on').value=b.oon;
  document.getElementById('osc-off').value=b.oof;
  onTypeChange();
}

function onTypeChange(){
  var t=parseInt(document.getElementById('type').value);
  B[sel].t=t;
  ['cc','note','pc','key','osc'].forEach(function(f,i){
    var el=document.getElementById('fields-'+f);
    el.className='field-group'+(i===t?' visible':'');
    el.setAttribute('aria-hidden',i!==t?'true':'false');
  });
  renderPedal();
}

function setBehavior(v){
  B[sel].bh=v;
  document.getElementById('beh-mom').setAttribute('aria-pressed',v===0?'true':'false');
  document.getElementById('beh-tog').setAttribute('aria-pressed',v===1?'true':'false');
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
    document.getElementById('mod-'+m).setAttribute('aria-pressed',(B[sel].km&(1<<i))?'true':'false');
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
  zlooper:[
    {t:0,bh:0,cr:0,cg:0,cb:180,cor:255,cog:0,cob:0,kc:0,km:0,cc:37,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:0,bh:0,cr:128,cg:0,cb:180,cor:0,cog:255,cob:0,kc:0,km:0,cc:38,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:0,bh:0,cr:0,cg:0,cb:180,cor:255,cog:255,cob:255,kc:0,km:0,cc:39,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0},
    {t:0,bh:0,cr:0,cg:0,cb:180,cor:255,cog:255,cob:255,kc:0,km:0,cc:40,mch:1,con:127,cof:0,nt:60,vel:127,pc:0,oa:'',oon:1,oof:0}
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
  looper:['Rec','FX/Mute','Undo','Reset'],
  zlooper:['Rec','FX/Mute','Undo','Reset'],
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
    b.type='button';
    b.textContent=labels[i];
    b.setAttribute('aria-label','Test '+labels[i]+' button');
    b.onclick=(function(idx){return function(){testPress(idx)}})(i);
    row.appendChild(b);
  }
}

async function testPress(btn){
  var st=document.getElementById('st');
  st.textContent='Sending...';
  var res=await fetch('/press?b='+btn);
  if(res.ok){
    var j=await res.json();
    st.textContent=j.msg||'Sent';
  } else {
    st.textContent='Error';
  }
  setTimeout(function(){st.textContent=''},800);
}

function applyMode(m){
  currentMode=m;
  if(MODES[m]){
    B=JSON.parse(JSON.stringify(MODES[m]));
    selectBtn(0);
  }
  renderTestButtons();
  fetch('/press?b=99');
}

async function saveAll(){
  readFields();
  var modeMap={custom:0,looper:1,zlooper:2,pageturner:3,pedalboard:4};
  var pname=document.getElementById('pedal-name').value.trim()||'TribePedal';
  var payload={mode:modeMap[currentMode]||0,name:pname,buttons:B};
  var res=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
  document.getElementById('st').textContent=res.ok?'Saved! Reboot pedal to apply new name.':'Error';
  if(res.ok){
    document.getElementById('mdns-hint').textContent=pname.toLowerCase().replace(/[^a-z0-9]/g,'')+'.local';
    setTimeout(function(){document.getElementById('st').textContent=''},3000);
  }
}

var MODE_NAMES=['custom','looper','zlooper','pageturner','pedalboard'];
currentMode=MODE_NAMES[initMode]||'looper';
document.getElementById('mode-sel').value=currentMode;
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
    page.replace("%MODE%", String(cfg.mode));
    page.replace("%PEDAL_NAME%", String(pedal_name));
    String lc = String(pedal_name);
    lc.toLowerCase();
    page.replace("%PEDAL_NAME_LC%", lc);
    return page;
}

// ── Route handlers ─────────────────────────────────────────
static void handleRoot() {
    webServer.send(200, "text/html", buildPage());
}

static void handleSave() {
    String body = webServer.arg("plain");

    // Parse mode from top-level object
    int modePos = body.indexOf("\"mode\":");
    if (modePos >= 0) {
        cfg.mode = (PedalMode)body.substring(modePos + 7).toInt();
        if (cfg.mode == MODE_LOOPER) looper_init();
    }

    // Parse pedal name
    int namePos = body.indexOf("\"name\":\"");
    if (namePos >= 0) {
        int start = namePos + 8;
        int end = body.indexOf('"', start);
        if (end > start && end - start < (int)sizeof(pedal_name)) {
            String n = body.substring(start, end);
            strncpy(pedal_name, n.c_str(), sizeof(pedal_name) - 1);
            pedal_name[sizeof(pedal_name) - 1] = 0;
        }
    }

    // Parse buttons array
    int idx = 0;
    int pos = body.indexOf("\"buttons\"");
    if (pos < 0) pos = 0;
    while (idx < NUM_BUTTONS && pos < (int)body.length()) {
        int objStart = body.indexOf('{', pos + 1);
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

static void handleManifest() {
    webServer.send(200, "application/manifest+json",
        "{\"name\":\"Tribe Pedal\",\"short_name\":\"Pedal\","
        "\"start_url\":\"/\",\"display\":\"standalone\","
        "\"background_color\":\"#111111\",\"theme_color\":\"#111111\","
        "\"icons\":[{\"src\":\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAAsTAAALEwEAmpwYAAAFeklEQVR4nO1Wa0yTVxh+e4e2tNwEK/TCRWih1EKho1Boy1UpgaICCgIqqEO8TAE3jDpk4ma8TVA2Lz+mIiPo5jbnDLuoi8ncry1bsriYTbP9WJZlbgnZAijyLKerIJty2eqfZU/yJt93vvc7z/Od9znvd4j+x/QRQ0SSaeRJichB3oY8JvcnaYT1NhEpJkmbk2ANuWUv1YwSkcirAoISK+5GLesFE0JEUY9I0Vmcyjvbe2woWhMLIgr2Jj9/lrkW6uIOqEsOIyBh4QARGcefkjW3Mmqw/kAqandbsGhDHBOg8qaAwND09TDYnUjOykV01VkEmWqGiPg2oS9voatBO7Kj146sVXakLS/EkuYEJkDnTQERCseziK7qQ/iCdsy2NSF6WR9C0hpGIvRBgy0nM5BXHY3yJj2YkOrtRiYg2ZsCjGF5OxE+vx3E4bDJIVWnIqqiBwr7FvhIfd1jLJZsSUDtriR2bf83hGLmOyKKJCI9EdUEGZfCL8I6RsTCN0SH8Pm7wBP5jY1lV0Sifl8Ku97q2Y5GzzyBbrdMgsTIopW/61c9D8Pa3TA1d8K87QRS27qRse9t2Dv74TjyITg83gQRfw2rS4WNh1Oxf6MPvjwjxbWjEry7X4KeNjGOPueLjeXCQSIyP0qAVJFWcNt57ibsHZdgXFaDwje/gaXtDJio2KWbEFlcBwGXB6VQCIGnFCxEHA7CPWNJ2Qps6rKg0MrH+T1i4FM5XEVx+LzbD0PX5HCm829N1tDCNQuqfrEf/gDGFevwVOvJCV9nk8tw2uHAxTgd3pkbDbNEgmyZH96aG41+bSxeS0+H2W+8HCzYCqyo0OFGrwyV+QLWQ8Kmqr8+tmLzYOH5b2FY2z42UQCPhx67DRe0sfjYE1c88eD+YmwMujMzIOFyx95jy89WYX2ZcMDTzqcFm6HhxZGCvhsICQtGoIyDbJkMl3TaMbLHxQV9PNKkUjd5dYEQo9flaK0TDRGRiWYCro9PmdKSfe+T41L3V+TJZVOSPwgmwGHiY/iaHF1bfEeIKJtmCrmUXBcPSEbvvC+DTsNFmFCIq9MgvxwbgwA+H5ePSPD6C2IoLPmDPJ4of0bkfD7ZuneKR5hrM4z8sXpun6OYUsDm2aHuXIkvB7I5SjjfuAl93Y7hmZTA0NnoM8SMw/btw45m5jqh0TyW/BW1Gj4PGZBFUuMhMENHLaof8DSkSRHxfJ1o4LNTMiwqScCVLgk4nImNRsbjoVOl+hv5QZVygvtZcDhcpO85B2PNaji6PoIyu/SHyX7TgetKhT8y137RLUNNuda9fa6fkKLELkDTsXR3k2ET8zkcbAoNdRMzX9SHzHKPcQVCRLlWQVvVjIQ1bW5y1sxYU7N19KPg7NcITcn56nGHFfPTJcI7rXWiu/s2+OBYiy/62sXoPyTBhjIhnumyIK1INeELU6USmCTisXu+WIqsV6/C9vJ7sO49j9TWU0hpOYqkpg4Y6tsRv3IbVPkVbEsmTlUK4UM/I2aexoYDZjjKI8bJJEEIy2+DKFAzQZQ8Jg8B8S52Xe15N84zTwAR+dA/RObql5JRujl+3JD+MoTltSJyySmIFYY/a87lQ1V0ELMzGzEdw80EpuWtie7DRlmjHs7aGGw9nQm1PnhYYWsejarsdZMqnXs9B5W1Xj8TaitbDEgsdcJRZ3cLKaiLGeEKuGXEFZSHWOrvs2Naeo4VcbZiBCfXMAECbwpQsuWvbU/F+kMWZC7WsH961vhjXm6QqWpYXdKFiMXHETiv/B55GYGuBh22nbHBlKP4lYjmPSInxT+++Dd2bvSPd7Gm41XwbYs197Xm4O+JSD1JnlYWnfOzNDLjO3oCKCAi/2nkhbB2/iQE0H8SfwDB5dfWAE9+FAAAAABJRU5ErkJggg==\",\"sizes\":\"32x32\",\"type\":\"image/png\"}]}"
    );
}

static void handleNotFound() {
    // Derive lowercase alphanumeric host from current pedal name
    char host[33];
    int hi = 0;
    for (int i = 0; pedal_name[i] && hi < 31; i++) {
        char c = pedal_name[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) host[hi++] = c;
    }
    host[hi] = 0;
    if (hi == 0) strcpy(host, "tribepedal");
    String loc = String("http://") + host + ".local/";
    webServer.sendHeader("Location", loc, true);
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
void config_load_name_early() {
    Preferences p;
    p.begin("pedal", true);
    String savedName = p.getString("name", "TribePedal");
    p.end();
    strncpy(pedal_name, savedName.c_str(), sizeof(pedal_name) - 1);
    pedal_name[sizeof(pedal_name) - 1] = 0;
}

const char* config_get_pedal_name() {
    return pedal_name;
}

void config_init() {
    prefs.begin("pedal", true);
    cfg.led_brightness = prefs.getUChar("bright", 30);
    cfg.mode = (PedalMode)prefs.getUChar("mode", MODE_LOOPER);
    String savedName = prefs.getString("name", "TribePedal");
    strncpy(pedal_name, savedName.c_str(), sizeof(pedal_name) - 1);
    pedal_name[sizeof(pedal_name) - 1] = 0;
    for (int i = 0; i < NUM_BUTTONS; i++) loadButton(i);
    prefs.end();

    // Derive mDNS hostname: lowercase, alphanumeric only
    char hostname[33];
    int hi = 0;
    for (int i = 0; pedal_name[i] && hi < 31; i++) {
        char c = pedal_name[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) hostname[hi++] = c;
    }
    hostname[hi] = 0;
    if (hi == 0) strcpy(hostname, "tribepedal");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(pedal_name);
    dnsServer.start(53, "*", WiFi.softAPIP());
    MDNS.begin(hostname);

    webServer.on("/", handleRoot);
    webServer.on("/save", HTTP_POST, handleSave);
    webServer.on("/press", handlePress);
    webServer.on("/manifest.webmanifest", handleManifest);
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
    prefs.putUChar("mode", cfg.mode);
    prefs.putString("name", pedal_name);
    for (int i = 0; i < NUM_BUTTONS; i++) saveButton(i);
    prefs.end();
}
