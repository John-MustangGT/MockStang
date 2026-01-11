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

#endif // WEB_INTERFACE_H
