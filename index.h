const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Geiger Counter</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .cpm {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
  }
  .uSv {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
  }
  .uSh {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
  }
  </style>
<title>ESP Geiger Counter</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>ESP Geiger Counter</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>CPM</h2>
      <p class="cpm">cpm: <span id="cpm">%CPM%</span></p>
      <h2>uSv</h2>
      <p class="uSv">uSv: <span id="uSv">%USV%</span></p>
      <h2>uSh</h2>
      <p class="uSh">uSh: <span id="uSh">%USH%</span></p>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}:8080/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    console.log(event.data);
      var jsonObject = JSON.parse(event.data);
      var cpm = jsonObject.cpm;
      var uSv = jsonObject.uSv;
      var uSh = jsonObject.uSh;
      document.getElementById('cpm').innerHTML = cpm;
      document.getElementById('uSv').innerHTML = uSv;
      document.getElementById('uSh').innerHTML = uSh;
  }
  function onLoad(event) {
    initWebSocket();
  }
  function toggle(){
    websocket.send('toggle');
  }
</script>
</body>
</html>
)rawliteral";
