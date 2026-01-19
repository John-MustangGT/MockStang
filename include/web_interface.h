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
<h3>📊 Connection Statistics</h3>
<div style="display:grid;grid-template-columns:repeat(2,1fr);gap:15px;margin-top:10px">
<div style="background:#1a1a1a;padding:10px;border-radius:4px">
<div style="font-size:11px;color:#888;margin-bottom:5px">OBD Client</div>
<div style="font-size:14px;font-weight:bold;color:#4a9eff" id="statsClientStatus">Disconnected</div>
<div style="font-size:11px;color:#666;margin-top:3px" id="statsClientIP">--</div>
</div>
<div style="background:#1a1a1a;padding:10px;border-radius:4px">
<div style="font-size:11px;color:#888;margin-bottom:5px">Uptime</div>
<div style="font-size:14px;font-weight:bold;color:#4a9eff" id="statsUptime">0s</div>
</div>
<div style="background:#1a1a1a;padding:10px;border-radius:4px">
<div style="font-size:11px;color:#888;margin-bottom:5px">Total Commands</div>
<div style="font-size:14px;font-weight:bold;color:#4a9eff" id="statsTotalCommands">0</div>
</div>
<div style="background:#1a1a1a;padding:10px;border-radius:4px">
<div style="font-size:11px;color:#888;margin-bottom:5px">Commands/Min</div>
<div style="font-size:14px;font-weight:bold;color:#4a9eff" id="statsCommandsPerMin">0</div>
</div>
</div>
<div style="margin-top:15px;background:#1a1a1a;padding:10px;border-radius:4px">
<div style="font-size:11px;color:#888;margin-bottom:8px">Command Breakdown</div>
<div style="display:grid;grid-template-columns:repeat(4,1fr);gap:10px;text-align:center">
<div>
<div style="font-size:18px;font-weight:bold;color:#4a9eff" id="statsMode01">0</div>
<div style="font-size:10px;color:#666">Mode 01</div>
</div>
<div>
<div style="font-size:18px;font-weight:bold;color:#4a9eff" id="statsMode03">0</div>
<div style="font-size:10px;color:#666">Mode 03</div>
</div>
<div>
<div style="font-size:18px;font-weight:bold;color:#4a9eff" id="statsMode09">0</div>
<div style="font-size:10px;color:#666">Mode 09</div>
</div>
<div>
<div style="font-size:18px;font-weight:bold;color:#4a9eff" id="statsAT">0</div>
<div style="font-size:10px;color:#666">AT Cmds</div>
</div>
</div>
</div>
<div style="margin-top:10px;padding:8px;background:#1a1a1a;border-radius:4px">
<div style="font-size:10px;color:#888">Last Command:</div>
<div style="font-size:11px;color:#4a9eff;font-family:monospace;margin-top:3px" id="statsLastCommand">--</div>
</div>
</div>

<div class="card">
<h3>🚗 Driving Simulator</h3>
<p style="color:#aaa;margin:10px 0">Automatically animate vehicle parameters</p>
<div style="display:flex;gap:10px;flex-wrap:wrap">
<button onclick="setDriveMode(0)" id="driveOff" style="background:#666">OFF</button>
<button onclick="setDriveMode(1)" id="driveGentle">GENTLE</button>
<button onclick="setDriveMode(2)" id="driveNormal">NORMAL</button>
<button onclick="setDriveMode(3)" id="driveSport">SPORT</button>
<button onclick="setDriveMode(4)" id="driveDrag">DRAG RACE</button>
</div>
<div id="driveStatus" style="margin-top:15px;padding:10px;background:#1a1a1a;border-radius:4px;font-family:monospace;font-size:12px;color:#888">
Simulator: OFF
</div>
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
<h3>MIL & Diagnostic Codes</h3>
<div class="slider-group">
<label>Check Engine Light:</label>
<button id="milBtn" onclick="toggleMIL()" style="width:80px;margin:0">OFF</button>
<span id="milStatus" style="margin-left:10px;color:#888">No codes</span>
</div>
<div style="margin-top:15px">
<h4 style="margin:10px 0">Add DTC:</h4>
<div style="display:flex;gap:10px;align-items:center">
<select id="dtcType" style="padding:8px;background:#1a1a1a;border:1px solid #444;color:#fff;border-radius:4px">
<option value="0">P0xxx (Powertrain)</option>
<option value="1">P1xxx (Manufacturer)</option>
<option value="2">P2xxx (Powertrain)</option>
<option value="3">P3xxx (Powertrain)</option>
<option value="C">C0xxx (Chassis)</option>
<option value="B">B0xxx (Body)</option>
<option value="U">U0xxx (Network)</option>
</select>
<input type="text" id="dtcCode" placeholder="0420" maxlength="4" style="width:100px;padding:8px;background:#1a1a1a;border:1px solid #444;color:#fff;border-radius:4px">
<button onclick="addDTC()">Add Code</button>
</div>
</div>
<div id="dtcList" style="margin-top:15px;font-family:monospace"></div>
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

    // Handle state updates from driving simulator
    if(msg.type==='state'){
      document.getElementById('rpm').value=msg.rpm;
      document.getElementById('rpmVal').innerText=msg.rpm;
      document.getElementById('speed').value=msg.speed;
      document.getElementById('speedVal').innerText=msg.speed;
      document.getElementById('coolant').value=msg.coolant;
      document.getElementById('coolantVal').innerText=msg.coolant;
      document.getElementById('intake').value=msg.intake;
      document.getElementById('intakeVal').innerText=msg.intake;
      document.getElementById('throttle').value=msg.throttle;
      document.getElementById('throttleVal').innerText=msg.throttle;
      document.getElementById('maf').value=msg.maf;
      document.getElementById('mafVal').innerText=(msg.maf/100).toFixed(2);
      document.getElementById('fuel').value=msg.fuel;
      document.getElementById('fuelVal').innerText=msg.fuel;
      document.getElementById('baro').value=msg.baro;
      document.getElementById('baroVal').innerText=msg.baro;
      return;
    }

    // Handle connection statistics updates
    if(msg.type==='stats'){
      document.getElementById('statsClientStatus').innerText=msg.clientConnected?'Connected':'Disconnected';
      document.getElementById('statsClientStatus').style.color=msg.clientConnected?'#4ade80':'#888';
      document.getElementById('statsClientIP').innerText=msg.clientIP||'--';

      // Format uptime
      var uptime=msg.uptime;
      var hours=Math.floor(uptime/3600);
      var mins=Math.floor((uptime%3600)/60);
      var secs=uptime%60;
      var uptimeStr=hours>0?(hours+'h '+mins+'m'):(mins>0?(mins+'m '+secs+'s'):(secs+'s'));
      document.getElementById('statsUptime').innerText=uptimeStr;

      document.getElementById('statsTotalCommands').innerText=msg.totalCommands;
      document.getElementById('statsCommandsPerMin').innerText=msg.commandsPerMinute;
      document.getElementById('statsMode01').innerText=msg.mode01Count;
      document.getElementById('statsMode03').innerText=msg.mode03Count;
      document.getElementById('statsMode09').innerText=msg.mode09Count;
      document.getElementById('statsAT').innerText=msg.atCommandCount;
      document.getElementById('statsLastCommand').innerText=msg.lastCommand||'--';
      return;
    }

    // Handle command/response logs
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

var milState=false;
var dtcs=[];

function toggleMIL(){
  milState=!milState;
  var btn=document.getElementById('milBtn');
  if(milState){
    btn.innerText='ON';
    btn.style.background='#dc2626';
  }else{
    btn.innerText='OFF';
    btn.style.background='#ff6b35';
  }
  if(ws && ws.readyState===WebSocket.OPEN){
    ws.send(JSON.stringify({cmd:'set_mil',value:milState}));
  }
  updateMILStatus();
}

function addDTC(){
  var type=document.getElementById('dtcType').value;
  var code=document.getElementById('dtcCode').value.trim();
  if(code.length!==4 || !/^[0-9A-F]+$/i.test(code)){
    alert('Please enter a valid 4-digit hex code (e.g., 0420)');
    return;
  }
  var dtcStr=type.toUpperCase()+code.toUpperCase();
  if(dtcs.includes(dtcStr)){
    alert('DTC already exists');
    return;
  }
  if(dtcs.length>=8){
    alert('Maximum 8 DTCs allowed');
    return;
  }
  dtcs.push(dtcStr);
  milState=true;
  document.getElementById('dtcCode').value='';
  if(ws && ws.readyState===WebSocket.OPEN){
    ws.send(JSON.stringify({cmd:'add_dtc',dtc:dtcStr}));
  }
  updateDTCList();
  updateMILStatus();
}

function removeDTC(dtc){
  var idx=dtcs.indexOf(dtc);
  if(idx>-1){
    dtcs.splice(idx,1);
    if(ws && ws.readyState===WebSocket.OPEN){
      ws.send(JSON.stringify({cmd:'remove_dtc',dtc:dtc}));
    }
    updateDTCList();
    updateMILStatus();
  }
}

function clearAllDTCs(){
  dtcs=[];
  milState=false;
  if(ws && ws.readyState===WebSocket.OPEN){
    ws.send(JSON.stringify({cmd:'clear_dtcs'}));
  }
  updateDTCList();
  updateMILStatus();
}

function updateMILStatus(){
  var btn=document.getElementById('milBtn');
  var status=document.getElementById('milStatus');
  if(milState){
    btn.innerText='ON';
    btn.style.background='#dc2626';
  }else{
    btn.innerText='OFF';
    btn.style.background='#ff6b35';
  }
  if(dtcs.length===0){
    status.innerText='No codes';
    status.style.color='#888';
  }else{
    status.innerText=dtcs.length+' code(s) stored';
    status.style.color='#ff6b35';
  }
}

function updateDTCList(){
  var list=document.getElementById('dtcList');
  if(dtcs.length===0){
    list.innerHTML='<div style="color:#888;font-style:italic">No DTCs stored</div>';
  }else{
    var html='<h4 style="margin:10px 0">Stored DTCs:</h4>';
    dtcs.forEach(function(dtc){
      html+='<div style="padding:5px;background:#1a1a1a;margin:5px 0;border-radius:4px;display:flex;justify-content:space-between;align-items:center">';
      html+='<span style="color:#ff6b35">'+dtc+'</span>';
      html+='<button onclick="removeDTC(\''+dtc+'\')" style="padding:5px 10px;font-size:12px">Remove</button>';
      html+='</div>';
    });
    html+='<button onclick="clearAllDTCs()" style="margin-top:10px;background:#dc2626">Clear All DTCs</button>';
    list.innerHTML=html;
  }
}

var currentDriveMode=0;

function setDriveMode(mode){
  currentDriveMode=mode;
  // Update button styles
  var modes=['driveOff','driveGentle','driveNormal','driveSport','driveDrag'];
  modes.forEach(function(id,idx){
    var btn=document.getElementById(id);
    if(idx===mode){
      btn.style.background='#16a34a';
    }else{
      btn.style.background=(idx===0)?'#666':'#ff6b35';
    }
  });
  // Update status
  var modeNames=['OFF','GENTLE (0-50 km/h, 5s, warming up)','NORMAL (0-80 km/h, 7s, cruise, stop)','SPORT (0-120 km/h, 8s, hard accel)','DRAG RACE (0-180 km/h, 12s, full throttle)'];
  document.getElementById('driveStatus').innerText='Simulator: '+modeNames[mode];
  // Send command
  if(ws && ws.readyState===WebSocket.OPEN){
    ws.send(JSON.stringify({cmd:'set_drive_mode',mode:mode}));
  }
}

window.onload=function(){
  initWebSocket();
  updateDTCList();
  updateMILStatus();
};
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
