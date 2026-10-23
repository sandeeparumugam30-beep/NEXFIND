
/*
  NEXFIND - Part A
  ESP8266 receiver + web dashboard

  Wi-Fi:
    SSID: NEXFIND_OBJECT
    Password: find12345

  Dashboard:
    http://192.168.4.50

  Buzzer:
    TX / GPIO1
    Active-LOW: LOW = ON, HIGH = OFF

  Seven-segment:
    Common-cathode display
    Segment mapping: a,b,c,d,e,f,g
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>

const char* WIFI_SSID = "NEXFIND_OBJECT";
const char* WIFI_PASSWORD = "find12345";
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

IPAddress localIP(192, 168, 4, 50);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

const unsigned int UDP_PORT = 4210;
const unsigned long SIGNAL_TIMEOUT_MS = 3500;

WiFiUDP udp;
ESP8266WebServer server(80);

// ---------------- WEB DASHBOARD ----------------

const char page[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NEXFIND | Signal Intelligence</title>
<style>
:root{
  --bg:#080d12;--panel:#111a22;--line:#25343e;
  --muted:#82949e;--text:#edf7f5;--green:#54f5b0;
  --cyan:#62d9f5;--red:#ff647c;--amber:#ffc96b;
}
*{box-sizing:border-box}
body{
 margin:0;background:var(--bg);color:var(--text);
 font-family:Inter,Segoe UI,Arial,sans-serif;
 background-image:radial-gradient(ellipse at 80% 0%,#12332b55,transparent 42%);
}
.shell{max-width:1250px;margin:auto;padding:26px}
header{display:flex;justify-content:space-between;align-items:center;
 gap:15px;border-bottom:1px solid var(--line);padding-bottom:22px}
.brand{display:flex;align-items:center;gap:13px}
.logo{width:45px;height:45px;border:1px solid #54f5b066;
 border-radius:13px;display:grid;place-items:center;color:var(--green);
 font-size:25px;background:#54f5b010}
h1{font-size:21px;letter-spacing:3px;margin:0}
.tag{font-size:10px;letter-spacing:2px;color:var(--muted);margin-top:6px}
.live{font-size:11px;letter-spacing:1px;color:var(--green);
 border:1px solid #54f5b044;border-radius:30px;padding:10px 13px}
.dot{display:inline-block;width:7px;height:7px;border-radius:50%;
 background:var(--green);margin-right:7px;box-shadow:0 0 10px var(--green)}
.intro{margin:27px 0 20px}
.eyebrow{color:var(--green);font-size:10px;letter-spacing:2px}
h2{font-size:clamp(25px,4vw,38px);margin:9px 0}
.intro p{color:var(--muted);font-size:13px;line-height:1.7;margin:0}
.grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:13px}
.card{background:linear-gradient(145deg,#14202a,#0f171e);
 border:1px solid var(--line);border-radius:15px;padding:19px;min-width:0}
.label{font-size:10px;color:var(--muted);letter-spacing:1.5px}
.value{font-size:29px;font-weight:650;letter-spacing:-1px;margin:16px 0 8px}
.unit{font-size:12px;color:var(--muted);font-weight:400;letter-spacing:0}
.note{font-size:11px;color:var(--muted);line-height:1.6}
.section{margin-top:18px}
.sectionhead{display:flex;align-items:center;justify-content:space-between;
 gap:10px;margin-bottom:13px}
.sectionhead h3{font-size:13px;letter-spacing:1.5px;margin:0}
.pill{font-size:10px;padding:7px 9px;border-radius:7px;
 background:#182a28;color:var(--green);border:1px solid #54f5b02a}
.mainGrid{display:grid;grid-template-columns:minmax(0,1.6fr) minmax(260px,1fr);
 gap:15px;margin-top:17px}
.map{
 position:relative;min-height:340px;overflow:hidden;border-radius:11px;
 background-color:#0b171b;
 background-image:linear-gradient(#1e454333 1px,transparent 1px),
 linear-gradient(90deg,#1e454333 1px,transparent 1px);
 background-size:27px 27px;
}
.map svg{position:absolute;inset:0;width:100%;height:100%}
.mapcaption{display:flex;justify-content:space-between;gap:10px;
 margin-top:13px;color:var(--muted);font-size:11px;line-height:1.6}
.legend{display:flex;gap:13px;flex-wrap:wrap}
.legend span:before{content:"";display:inline-block;width:7px;height:7px;
 border-radius:50%;margin-right:6px;background:var(--green)}
.legend span:nth-child(2):before{background:var(--cyan)}
.legend span:nth-child(3):before{background:var(--amber)}
.metric{display:flex;justify-content:space-between;gap:12px;
 padding:14px 0;border-bottom:1px solid var(--line);font-size:12px}
.metric:last-child{border:0}
.metric span:first-child{color:var(--muted)}
.metric strong{font-weight:550;text-align:right;overflow-wrap:anywhere}
.bar{height:5px;border-radius:8px;background:#26343c;overflow:hidden;margin:12px 0}
.fill{height:100%;width:0;background:var(--green);transition:width .5s}
.historyrow{display:flex;align-items:center;gap:10px;margin-top:13px;font-size:11px}
.historybar{height:5px;flex:1;background:#26343c;border-radius:5px;overflow:hidden}
.historybar div{height:100%;background:var(--cyan)}
.historyval{width:62px;text-align:right;color:var(--muted)}
.footer{margin:22px 0 5px;padding-top:17px;border-top:1px solid var(--line);
 color:var(--muted);font-size:10px;line-height:1.8;letter-spacing:.4px}
@media(max-width:800px){
 .shell{padding:16px}.grid{grid-template-columns:repeat(2,minmax(0,1fr))}
 .mainGrid{grid-template-columns:1fr}.map{min-height:300px}
}
@media(max-width:400px){
 .shell{padding:11px}.card{padding:13px}.value{font-size:24px}
 h1{font-size:17px}.live{font-size:9px;padding:8px}
}
</style>
</head>
<body>
<div class="shell">
<header>
 <div class="brand">
  <div class="logo">⌁</div>
  <div><h1>NEXFIND</h1><div class="tag">SIGNAL INTELLIGENCE SYSTEM</div></div>
 </div>
 <div class="live"><span class="dot" id="liveDot"></span><span id="status">CONNECTING</span></div>
</header>

<div class="intro">
 <div class="eyebrow">FIELD OPERATIONS / 01</div>
 <h2>Object signal intelligence.</h2>
 <p>Live receiver telemetry · Wireless proximity guidance · Local monitoring</p>
</div>

<div class="grid">
 <div class="card">
  <div class="label">LIVE SIGNAL</div>
  <div class="value"><span id="rssi">--</span> <span class="unit">dBm</span></div>
  <div class="note">Current Wi-Fi link strength</div>
 </div>
 <div class="card">
  <div class="label">SIGNAL INDEX</div>
  <div class="value"><span id="score">--</span><span class="unit"> / 9</span></div>
  <div class="bar"><div class="fill" id="bar"></div></div>
  <div class="note" id="quality">Awaiting telemetry</div>
 </div>
 <div class="card">
  <div class="label">BEST RECORDED</div>
  <div class="value"><span id="best">--</span> <span class="unit">dBm</span></div>
  <div class="note">Strongest session reading</div>
 </div>
 <div class="card">
  <div class="label">VALID PACKETS</div>
  <div class="value" id="packets">0</div>
  <div class="note">Timeouts: <span id="timeouts">0</span></div>
 </div>
</div>

<div class="mainGrid">
 <section class="card">
  <div class="sectionhead">
   <h3>PROXIMITY SEARCH FIELD</h3>
   <span class="pill" id="mapState">STANDBY</span>
  </div>

  <div class="map">
   <svg viewBox="0 0 600 350" preserveAspectRatio="xMidYMid meet">
    <defs>
     <radialGradient id="halo">
      <stop offset="0%" stop-color="#54f5b0" stop-opacity=".23"/>
      <stop offset="100%" stop-color="#54f5b0" stop-opacity="0"/>
     </radialGradient>
     <filter id="glow">
      <feGaussianBlur stdDeviation="5" result="blur"/>
      <feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge>
     </filter>
    </defs>

    <circle cx="365" cy="160" r="135" fill="url(#halo)"/>
    <circle cx="365" cy="160" r="105" fill="none" stroke="#54f5b0"
     stroke-opacity=".16" stroke-dasharray="3 8"/>
    <circle cx="365" cy="160" r="75" fill="none" stroke="#54f5b0"
     stroke-opacity=".26" stroke-dasharray="3 7"/>
    <circle cx="365" cy="160" r="44" fill="none" stroke="#54f5b0"
     stroke-opacity=".4" stroke-dasharray="3 6"/>

    <path d="M30 70 L150 70 L150 130 L245 130 L245 45 L520 45
             M65 285 L65 220 L180 220 L180 300 L310 300
             M450 90 L540 90 L540 240 L460 240 L460 310
             M280 45 L280 100 L330 100
             M30 170 L100 170 L100 130"
     fill="none" stroke="#31534e" stroke-width="12" stroke-linejoin="round"/>

    <path d="M30 70 L150 70 L150 130 L245 130 L245 45 L520 45
             M65 285 L65 220 L180 220 L180 300 L310 300
             M450 90 L540 90 L540 240 L460 240 L460 310
             M280 45 L280 100 L330 100
             M30 170 L100 170 L100 130"
     fill="none" stroke="#547c70" stroke-width="1.5" stroke-linejoin="round"/>

    <path d="M90 260 Q190 235 275 195 T365 160"
     fill="none" stroke="#62d9f5" stroke-width="2"
     stroke-dasharray="5 7" opacity=".85"/>

    <circle cx="90" cy="260" r="19" fill="#62d9f5" opacity=".12"/>
    <circle cx="90" cy="260" r="8" fill="#62d9f5" filter="url(#glow)"/>
    <text x="90" y="292" text-anchor="middle" fill="#bcefff" font-size="11">RECEIVER</text>

    <g id="target" transform="translate(365 160)">
     <circle r="25" fill="#54f5b0" opacity=".12"/>
     <circle r="13" fill="#54f5b0" opacity=".18"/>
     <circle r="6" fill="#54f5b0" filter="url(#glow)"/>
     <path d="M0 -32 V-17 M0 17 V32 M-32 0 H-17 M17 0 H32"
      stroke="#54f5b0" stroke-width="1.5"/>
    </g>
    <text id="targetLabel" x="365" y="119" text-anchor="middle"
     fill="#a5ffda" font-size="12" letter-spacing="2">SIGNAL GUIDE</text>
    <text x="22" y="25" fill="#82949e" font-size="10" letter-spacing="1.5">SCHEMATIC VIEW</text>
   </svg>
  </div>

  <div class="mapcaption">
   <span id="mapMessage">Waiting for transmitter signal.</span>
   <span>NOT TO SCALE</span>
  </div>
  <div class="mapcaption legend">
   <span>Strong</span><span>Receiver</span><span>Weak</span>
  </div>
 </section>

 <section class="card">
  <div class="sectionhead">
   <h3>LIVE DIAGNOSTICS</h3>
   <span class="pill">UDP / 4210</span>
  </div>
  <div class="metric"><span>Connection</span><strong id="conn">Waiting</strong></div>
  <div class="metric"><span>Signal trend</span><strong id="trend">WAITING</strong></div>
  <div class="metric"><span>Current score</span><strong id="scoreSide">0 / 9</strong></div>
  <div class="metric"><span>Lost-signal events</span><strong id="timeoutsSide">0</strong></div>
  <div class="metric"><span>Invalid packets</span><strong id="invalid">0</strong></div>

  <div class="sectionhead" style="margin-top:24px">
   <h3>RECENT READINGS</h3><span class="note">LAST 5</span>
  </div>
  <div id="history"><div class="note">No readings received yet.</div></div>
 </section>
</div>

<div class="footer">
 NEXFIND / LOCAL EDGE DASHBOARD<br>
 SIGNAL STRENGTH IS AN ESTIMATE OF WIRELESS LINK QUALITY.
 THE SEARCH FIELD IS A VISUAL GUIDE, NOT A GPS POSITION OR MEASURED MAP.
</div>
</div>

<script>
async function update(){
 try{
  const response=await fetch('/data',{cache:'no-store'});
  if(!response.ok) throw new Error('HTTP');
  const d=await response.json();

  document.getElementById('status').textContent=d.active?'SIGNAL ACTIVE':'SIGNAL LOST';
  document.getElementById('liveDot').style.background=d.active?'#54f5b0':'#ff647c';
  document.getElementById('rssi').textContent=d.active?d.rssi:'--';
  document.getElementById('score').textContent=d.active?d.score:0;
  document.getElementById('best').textContent=d.strongest===-100?'--':d.strongest;
  document.getElementById('packets').textContent=d.packets;
  document.getElementById('timeouts').textContent=d.timeouts;
  document.getElementById('timeoutsSide').textContent=d.timeouts;
  document.getElementById('invalid').textContent=d.invalid===undefined?'--':d.invalid;
  document.getElementById('trend').textContent=d.trend;
  document.getElementById('conn').textContent=d.active?'RECEIVING':'NO RECENT SIGNAL';
  document.getElementById('scoreSide').textContent=(d.active?d.score:0)+' / 9';

  const s=d.active?d.score:0;
  document.getElementById('bar').style.width=(s*100/9)+'%';
  document.getElementById('bar').style.background=s<=3?'#ff647c':s<=6?'#ffc96b':'#54f5b0';
  document.getElementById('quality').textContent=!d.active?'No active signal':s<=3?'Weak link quality':s<=6?'Moderate link quality':'Strong link quality';

  const target=document.getElementById('target');
  const label=document.getElementById('targetLabel');
  const mapState=document.getElementById('mapState');

  if(!d.active){
   target.setAttribute('transform','translate(365 160)');
   label.setAttribute('x','365');
   label.setAttribute('y','119');
   label.textContent='SIGNAL GUIDE';
   mapState.textContent='STANDBY';
   mapState.style.color='#ff647c';
   document.getElementById('mapMessage').textContent='No recent signal. Check transmitter power and connection.';
  }else{
   const x=d.score>=7?330:d.score>=4?365:400;
   const y=d.score>=7?145:d.score>=4?160:175;
   target.setAttribute('transform','translate('+x+' '+y+')');
   label.setAttribute('x',x);
   label.setAttribute('y',y-41);
   label.textContent=d.score>=7?'STRONG LINK':d.score>=4?'MEDIUM LINK':'WEAK LINK';
   mapState.textContent=d.score>=7?'STRONG SIGNAL':d.score>=4?'MODERATE':'WEAK SIGNAL';
   mapState.style.color=d.score>=7?'#54f5b0':d.score>=4?'#ffc96b':'#ff647c';
   document.getElementById('mapMessage').textContent=
    d.score>=7?'Strong link quality. Compare readings as you move.':
    d.score>=4?'Moderate link. Move carefully and compare signal changes.':
    'Weak link. Check other areas and compare readings.';
  }

  let html='';
  if(!d.history || !d.history.length){
   html='<div class="note">No readings received yet.</div>';
  }else{
   d.history.forEach((v,i)=>{
    const strength=Math.max(4,Math.min(100,(v+100)*2));
    html+='<div class="historyrow"><span>'+(i+1)+'</span>'+
     '<div class="historybar"><div style="width:'+strength+'%"></div></div>'+
     '<span class="historyval">'+v+' dBm</span></div>';
   });
  }
  document.getElementById('history').innerHTML=html;
 }catch(e){
  document.getElementById('status').textContent='RECONNECTING';
  document.getElementById('conn').textContent='Dashboard retrying';
 }
}
update();
setInterval(update,1000);
</script>
</body>
</html>
)HTML";

// ---------------- HARDWARE PINS ----------------

// Seven-segment pins: a,b,c,d,e,f,g
// Note: GPIO3 is RX and GPIO1 is TX.
const int segPins[7] = {D4, D3, D2, D1, D0, 3, D8};

const int redLed = D5;
const int yellowLed = D6;
const int greenLed = D7;

const int buzzerPin = 1;  // TX / GPIO1, active-LOW

// Common-cathode display patterns: a,b,c,d,e,f,g
const byte digitPatterns[10] = {
  0b1111110, // 0
  0b0110000, // 1
  0b1101101, // 2
  0b1111001, // 3
  0b0110011, // 4
  0b1011011, // 5
  0b1011111, // 6
  0b1110000, // 7
  0b1111111, // 8
  0b1111011  // 9
};

// ---------------- VARIABLES ----------------

bool active = false;
int currentRSSI = -100;
int strongestRSSI = -100;
int score = 0;

unsigned long lastPacketAt = 0;
unsigned long packets = 0;
unsigned long timeouts = 0;
unsigned long invalidPackets = 0;

String trend = "WAITING";
int previousRSSI = -100;

int history[5] = {-100, -100, -100, -100, -100};
int historyCount = 0;

// ---------------- DISPLAY ----------------

void showDigit(int n) {
  if (n < 0 || n > 9) {
    n = 0;
  }

  byte pattern = digitPatterns[n];

  for (int i = 0; i < 7; i++) {
    bool on = (pattern & (1 << (6 - i))) != 0;
    digitalWrite(segPins[i], on ? HIGH : LOW);
  }
}

// ---------------- SIGNAL SCORE ----------------

int calculateScore(int rssi) {
  if (rssi >= -45) return 9;
  if (rssi >= -50) return 8;
  if (rssi >= -55) return 7;
  if (rssi >= -60) return 6;
  if (rssi >= -65) return 5;
  if (rssi >= -70) return 4;
  if (rssi >= -75) return 3;
  if (rssi >= -82) return 2;

  return 1;
}

// ---------------- LED + BUZZER OUTPUTS ----------------

void updateOutputs() {

  // SIGNAL LOST
  if (!active) {
    digitalWrite(redLed, LOW);
    digitalWrite(yellowLed, LOW);
    digitalWrite(greenLed, LOW);

    digitalWrite(buzzerPin, HIGH);  // Buzzer OFF
    buzzerState = false;

    showDigit(0);
    return;
  }

  showDigit(score);

  // RED: Weak signal, buzzer OFF
  if (score <= 3) {
    digitalWrite(redLed, HIGH);
    digitalWrite(yellowLed, LOW);
    digitalWrite(greenLed, LOW);

    digitalWrite(buzzerPin, HIGH);
    buzzerState = false;
  }

  // YELLOW: Moderate signal, tik-tik sound
  else if (score <= 6) {
    digitalWrite(redLed, LOW);
    digitalWrite(yellowLed, HIGH);
    digitalWrite(greenLed, LOW);

    if (millis() - lastBuzzerToggle >= 40) {
      lastBuzzerToggle = millis();
      buzzerState = !buzzerState;

      digitalWrite(buzzerPin, buzzerState ? LOW : HIGH);
    }
  }

  // GREEN: Strong signal, continuous sound
  else {
    digitalWrite(redLed, LOW);
    digitalWrite(yellowLed, LOW);
    digitalWrite(greenLed, HIGH);

    digitalWrite(buzzerPin, LOW);  // Continuous ON
    buzzerState = true;
  }
}
// ---------------- READING HISTORY ----------------

void addHistory(int value) {
  for (int i = 4; i > 0; i--) {
    history[i] = history[i - 1];
  }

  history[0] = value;

  if (historyCount < 5) {
    historyCount++;
  }
}

// ---------------- WEB HANDLERS ----------------

void handleRoot() {
  server.send_P(200, "text/html", page);
}

void handleData() {
  String json = "{";

  json += "\"active\":" + String(active ? "true" : "false") + ",";
  json += "\"rssi\":" + String(active ? currentRSSI : -100) + ",";
  json += "\"score\":" + String(active ? score : 0) + ",";
  json += "\"strongest\":" + String(strongestRSSI) + ",";
  json += "\"trend\":\"" + trend + "\",";
  json += "\"packets\":" + String(packets) + ",";
  json += "\"timeouts\":" + String(timeouts) + ",";
  json += "\"invalid\":" + String(invalidPackets) + ",";
  json += "\"history\":[";

  for (int i = 0; i < historyCount; i++) {
    if (i > 0) {
      json += ",";
    }

    json += String(history[i]);
  }

  json += "]}";

  server.send(200, "application/json", json);
}

// ---------------- SETUP ----------------

void setup() {

  // Configure seven-segment pins
  for (int i = 0; i < 7; i++) {
    pinMode(segPins[i], OUTPUT);
    digitalWrite(segPins[i], LOW);
  }

  // Configure LEDs
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  // Configure active-LOW buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);  // OFF

  // Start with all LEDs OFF
  digitalWrite(redLed, LOW);
  digitalWrite(yellowLed, LOW);
  digitalWrite(greenLed, LOW);

  showDigit(0);

  delay(200);

  // Connect to Part B's Wi-Fi access point
  WiFi.mode(WIFI_STA);
  WiFi.config(localIP, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttempt < 20000) {
    delay(300);
  }

  // Start UDP receiver
  udp.begin(UDP_PORT);

  // Start dashboard
  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.onNotFound([]() {
    server.send(404, "text/plain", "NEXFIND: endpoint not found");
  });

  server.begin();

  // Make sure outputs reflect initial inactive state
  active = false;
  updateOutputs();
}

// ---------------- MAIN LOOP ----------------

void loop() {

  // Reconnect if Wi-Fi disconnects
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnect = 0;

    if (millis() - lastReconnect > 5000) {
      lastReconnect = millis();

      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }

  // Handle browser requests
  server.handleClient();

  // Receive UDP packets from Part B
  int packetSize = udp.parsePacket();

  if (packetSize > 0) {
    char incoming[100];

    int len = udp.read(incoming, sizeof(incoming) - 1);

    if (len < 0) {
      len = 0;
    }

    incoming[len] = '\0';

    String message = String(incoming);
    message.trim();

    if (message == "OBJECT_PRESENT") {

      lastPacketAt = millis();

      // Read transmitter Wi-Fi signal strength
      currentRSSI = WiFi.RSSI();

      score = calculateScore(currentRSSI);

      if (currentRSSI > strongestRSSI) {
        strongestRSSI = currentRSSI;
      }

      // Determine signal trend
      if (previousRSSI == -100) {
        trend = "STABLE";
      }
      else if (currentRSSI > previousRSSI + 2) {
        trend = "RISING";
      }
      else if (currentRSSI < previousRSSI - 2) {
        trend = "FALLING";
      }
      else {
        trend = "STABLE";
      }

      previousRSSI = currentRSSI;

      packets++;

      addHistory(currentRSSI);

      active = true;

      // Update display, LEDs and buzzer
      updateOutputs();

    } else {
      invalidPackets++;
    }
  }

  // Signal timeout: transmitter has stopped sending packets
  if (active && millis() - lastPacketAt > SIGNAL_TIMEOUT_MS) {

    active = false;

    timeouts++;

    trend = "SIGNAL LOST";

    // Turns all LEDs OFF and buzzer OFF
    updateOutputs();
  }

  delay(2);
}
