#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MockStang Dashboard</title>
<style>
body{font-family:Arial;margin:0;padding:20px;background:#1a1a1a;color:#fff}
h1{color:#ff6b35;margin-top:0}
.container{max-width:800px;margin:0 auto}
.card{background:#2d2d2d;padding:15px;margin:10px 0;border-radius:8px;border-left:4px solid #ff6b35}
.slider-group{margin:10px 0}
.slider-group label{display:inline-block;width:150px}
.slider-group input{width:200px}
.slider-group span{display:inline-block;width:80px;text-align:right;font-weight:bold;color:#ff6b35}
#log{background:#000;padding:10px;height:300px;overflow-y:auto;font-family:monospace;font-size:12px;border:1px solid #444;border-radius:4px}
.log-cmd{color:#4ade80}
.log-resp{color:#60a5fa}
.log-time{color:#888}
.status{display:inline-block;padding:5px 10px;border-radius:4px;font-size:14px;margin-left:10px}
.status.connected{background:#16a34a;color:#fff}
.status.disconnected{background:#dc2626;color:#fff}
button{background:#ff6b35;color:#fff;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;font-size:14px;margin:5px}
button:hover{background:#ff8555}
</style>
</head>
<body>
<div class="container">
<h1>🏎️ MockStang Dashboard</h1>

<div class="card">
<h3>Connection Status <span id="wsStatus" class="status disconnected">Disconnected</span></h3>
<button onclick="resetRuntime()">Reset Runtime</button>
<button onclick="clearLog()">Clear Log</button>
<button onclick="window.location.href='/settings'">⚙️ Settings</button>
</div>

<div class="card">
<h3>Car Parameters</h3>
<div class="slider-group">
<label>RPM:</label>
<input type="range" min="0" max="7000" value="850" id="rpm" oninput="updateValue('rpm')">
<span id="rpmVal">850</span>
</div>
<div class="slider-group">
<label>Speed (km/h):</label>
<input type="range" min="0" max="200" value="0" id="speed" oninput="updateValue('speed')">
<span id="speedVal">0</span>
</div>
<div class="slider-group">
<label>Coolant Temp (°C):</label>
<input type="range" min="-40" max="150" value="90" id="coolant" oninput="updateValue('coolant')">
<span id="coolantVal">90</span>
</div>
<div class="slider-group">
<label>Intake Temp (°C):</label>
<input type="range" min="-40" max="150" value="25" id="intake" oninput="updateValue('intake')">
<span id="intakeVal">25</span>
</div>
<div class="slider-group">
<label>Throttle (%):</label>
<input type="range" min="0" max="100" value="0" id="throttle" oninput="updateValue('throttle')">
<span id="throttleVal">0</span>
</div>
<div class="slider-group">
<label>MAF (g/s):</label>
<input type="range" min="0" max="1000" value="250" id="maf" oninput="updateValue('maf')">
<span id="mafVal">2.50</span>
</div>
<div class="slider-group">
<label>Fuel Level (%):</label>
<input type="range" min="0" max="100" value="75" id="fuel" oninput="updateValue('fuel')">
<span id="fuelVal">75</span>
</div>
<div class="slider-group">
<label>Barometric (kPa):</label>
<input type="range" min="80" max="110" value="101" id="baro" oninput="updateValue('baro')">
<span id="baroVal">101</span>
</div>
</div>

<div class="card">
<h3>OBD-II Activity Monitor</h3>
<div id="log"></div>
</div>
</div>

<script>
var ws;
function initWebSocket(){
  ws=new WebSocket('ws://'+window.location.hostname+'/ws');
  ws.onopen=function(){
    document.getElementById('wsStatus').className='status connected';
    document.getElementById('wsStatus').innerText='Connected';
  };
  ws.onclose=function(){
    document.getElementById('wsStatus').className='status disconnected';
    document.getElementById('wsStatus').innerText='Disconnected';
    setTimeout(initWebSocket,2000);
  };
  ws.onmessage=function(e){
    var log=document.getElementById('log');
    var msg=JSON.parse(e.data);
    var now=new Date().toLocaleTimeString();
    if(msg.cmd){
      log.innerHTML+='<span class="log-time">['+now+']</span> <span class="log-cmd">← '+msg.cmd+'</span><br>';
    }
    if(msg.resp){
      log.innerHTML+='<span class="log-time">['+now+']</span> <span class="log-resp">→ '+msg.resp.replace(/\r/g,'\\r')+'</span><br>';
    }
    log.scrollTop=log.scrollHeight;
  };
}

function updateValue(param){
  var val=document.getElementById(param).value;
  if(param=='maf'){
    document.getElementById(param+'Val').innerText=(val/100).toFixed(2);
  }else{
    document.getElementById(param+'Val').innerText=val;
  }
  if(ws && ws.readyState===WebSocket.OPEN){
    ws.send(JSON.stringify({param:param,value:parseInt(val)}));
  }
}

function resetRuntime(){
  if(ws && ws.readyState===WebSocket.OPEN){
    ws.send(JSON.stringify({cmd:'reset_runtime'}));
  }
}

function clearLog(){
  document.getElementById('log').innerHTML='';
}

window.onload=initWebSocket;
</script>
</body>
</html>
)rawliteral";

const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MockStang Settings</title>
<style>
body{font-family:Arial;margin:0;padding:20px;background:#1a1a1a;color:#fff}
h1{color:#ff6b35;margin-top:0}
.container{max-width:800px;margin:0 auto}
.card{background:#2d2d2d;padding:15px;margin:10px 0;border-radius:8px;border-left:4px solid #ff6b35}
.form-group{margin:15px 0}
.form-group label{display:block;margin-bottom:5px;color:#aaa}
.form-group input[type="text"],.form-group input[type="number"]{width:100%;padding:8px;background:#1a1a1a;border:1px solid #444;color:#fff;border-radius:4px;box-sizing:border-box}
.form-group input[type="checkbox"]{width:20px;height:20px;margin-right:10px;vertical-align:middle}
button{background:#ff6b35;color:#fff;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;font-size:14px;margin:5px}
button:hover{background:#ff8555}
button.secondary{background:#444}
button.secondary:hover{background:#555}
button.danger{background:#dc2626}
button.danger:hover{background:#ef4444}
.status{padding:10px;margin:10px 0;border-radius:4px;display:none}
.status.success{background:#16a34a;display:block}
.status.error{background:#dc2626;display:block}
</style>
</head>
<body>
<div class="container">
<h1>⚙️ MockStang Settings</h1>

<div id="statusMsg" class="status"></div>

<div class="card">
<h3>Network Configuration</h3>
<div class="form-group">
<label><input type="checkbox" id="useCustomSSID" onchange="toggleCustomSSID()"> Use Custom SSID</label>
<small style="color:#888">If unchecked, SSID will be auto-generated as iCAR_PRO_XXXX from MAC address</small>
</div>
<div class="form-group" id="customSSIDGroup" style="display:none">
<label>Custom SSID:</label>
<input type="text" id="ssid" maxlength="31" placeholder="iCAR_PRO_XXXX">
</div>
<div class="form-group">
<label><input type="checkbox" id="usePassword" onchange="togglePassword()"> Enable WiFi Password</label>
</div>
<div class="form-group" id="passwordGroup" style="display:none">
<label>WiFi Password:</label>
<input type="text" id="password" maxlength="63" placeholder="Enter password">
</div>
<div class="form-group">
<label>IP Address:</label>
<input type="text" id="ip" placeholder="192.168.0.10" pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
</div>
<div class="form-group">
<label>Subnet Mask:</label>
<input type="text" id="subnet" placeholder="255.255.255.0" pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
</div>
<div class="form-group">
<label>Gateway:</label>
<input type="text" id="gateway" placeholder="192.168.0.10" pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
</div>
</div>

<div class="card">
<h3>Vehicle Information</h3>
<div class="form-group">
<label>VIN (Vehicle Identification Number):</label>
<input type="text" id="vin" maxlength="17" placeholder="1ZVBP8AM5D5123456" style="text-transform:uppercase">
<small style="color:#888">17 characters - used for OBD-II mode 09 requests</small>
</div>
<div class="form-group">
<label>Device ID:</label>
<input type="text" id="deviceId" maxlength="31" placeholder="MockStang ESP-01S">
<small style="color:#888">Returned by AT@1 command</small>
</div>
</div>

<div class="card">
<h3>Default PID Values</h3>
<small style="color:#888">These values will be used when MockStang starts up</small>
<div class="form-group">
<label>Default RPM:</label>
<input type="number" id="defaultRPM" min="0" max="7000" value="850">
</div>
<div class="form-group">
<label>Default Speed (km/h):</label>
<input type="number" id="defaultSpeed" min="0" max="255" value="0">
</div>
<div class="form-group">
<label>Default Coolant Temp (°C):</label>
<input type="number" id="defaultCoolantTemp" min="-40" max="150" value="90">
</div>
<div class="form-group">
<label>Default Intake Temp (°C):</label>
<input type="number" id="defaultIntakeTemp" min="-40" max="150" value="25">
</div>
<div class="form-group">
<label>Default Throttle (%):</label>
<input type="number" id="defaultThrottle" min="0" max="100" value="0">
</div>
<div class="form-group">
<label>Default MAF (g/s × 100):</label>
<input type="number" id="defaultMAF" min="0" max="65535" value="250">
<small style="color:#888">Displayed value = input / 100 (e.g., 250 = 2.50 g/s)</small>
</div>
<div class="form-group">
<label>Default Fuel Level (%):</label>
<input type="number" id="defaultFuelLevel" min="0" max="100" value="75">
</div>
<div class="form-group">
<label>Default Barometric Pressure (kPa):</label>
<input type="number" id="defaultBarometric" min="80" max="110" value="101">
</div>
</div>

<div class="card">
<button onclick="saveConfig()">💾 Save Configuration</button>
<button class="secondary" onclick="window.location.href='/'">← Back to Dashboard</button>
<button class="danger" onclick="resetConfig()">🔄 Factory Reset</button>
</div>
</div>

<script>
function toggleCustomSSID(){
  var checked=document.getElementById('useCustomSSID').checked;
  document.getElementById('customSSIDGroup').style.display=checked?'block':'none';
}

function togglePassword(){
  var checked=document.getElementById('usePassword').checked;
  document.getElementById('passwordGroup').style.display=checked?'block':'none';
}

function showStatus(msg,success){
  var el=document.getElementById('statusMsg');
  el.innerText=msg;
  el.className=success?'status success':'status error';
  setTimeout(function(){el.style.display='none'},5000);
}

function loadConfig(){
  fetch('/api/config')
    .then(r=>r.json())
    .then(cfg=>{
      document.getElementById('useCustomSSID').checked=cfg.useCustomSSID;
      document.getElementById('ssid').value=cfg.ssid;
      document.getElementById('usePassword').checked=cfg.usePassword;
      document.getElementById('password').value=cfg.password;
      document.getElementById('ip').value=cfg.ip;
      document.getElementById('subnet').value=cfg.subnet;
      document.getElementById('gateway').value=cfg.gateway;
      document.getElementById('vin').value=cfg.vin;
      document.getElementById('deviceId').value=cfg.deviceId;
      document.getElementById('defaultRPM').value=cfg.defaultRPM;
      document.getElementById('defaultSpeed').value=cfg.defaultSpeed;
      document.getElementById('defaultCoolantTemp').value=cfg.defaultCoolantTemp;
      document.getElementById('defaultIntakeTemp').value=cfg.defaultIntakeTemp;
      document.getElementById('defaultThrottle').value=cfg.defaultThrottle;
      document.getElementById('defaultMAF').value=cfg.defaultMAF;
      document.getElementById('defaultFuelLevel').value=cfg.defaultFuelLevel;
      document.getElementById('defaultBarometric').value=cfg.defaultBarometric;
      toggleCustomSSID();
      togglePassword();
    })
    .catch(e=>showStatus('Failed to load configuration',false));
}

function saveConfig(){
  var cfg={
    useCustomSSID:document.getElementById('useCustomSSID').checked,
    ssid:document.getElementById('ssid').value,
    usePassword:document.getElementById('usePassword').checked,
    password:document.getElementById('password').value,
    ip:document.getElementById('ip').value,
    subnet:document.getElementById('subnet').value,
    gateway:document.getElementById('gateway').value,
    vin:document.getElementById('vin').value.toUpperCase(),
    deviceId:document.getElementById('deviceId').value,
    defaultRPM:parseInt(document.getElementById('defaultRPM').value),
    defaultSpeed:parseInt(document.getElementById('defaultSpeed').value),
    defaultCoolantTemp:parseInt(document.getElementById('defaultCoolantTemp').value),
    defaultIntakeTemp:parseInt(document.getElementById('defaultIntakeTemp').value),
    defaultThrottle:parseInt(document.getElementById('defaultThrottle').value),
    defaultMAF:parseInt(document.getElementById('defaultMAF').value),
    defaultFuelLevel:parseInt(document.getElementById('defaultFuelLevel').value),
    defaultBarometric:parseInt(document.getElementById('defaultBarometric').value)
  };

  fetch('/api/config',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(cfg)
  })
  .then(r=>r.json())
  .then(d=>{
    if(d.success){
      showStatus('✓ Configuration saved! Restart MockStang to apply changes.',true);
    }else{
      showStatus('✗ Failed to save configuration',false);
    }
  })
  .catch(e=>showStatus('✗ Error: '+e,false));
}

function resetConfig(){
  if(!confirm('Reset all settings to factory defaults? This cannot be undone!')){
    return;
  }
  fetch('/api/reset',{method:'POST'})
    .then(r=>r.json())
    .then(d=>{
      if(d.success){
        showStatus('✓ Configuration reset! Restart MockStang to apply changes.',true);
        setTimeout(loadConfig,1000);
      }else{
        showStatus('✗ Failed to reset configuration',false);
      }
    })
    .catch(e=>showStatus('✗ Error: '+e,false));
}

window.onload=loadConfig;
</script>
</body>
</html>
)rawliteral";

#endif // WEB_INTERFACE_H
