#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <WiFiManager.h>
#define WEBSERVER_H
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoOTA.h>
#include "wifi.h"
#include "index.h"

/// Geiger counter
#define RECEIVER_PIN D7  // 4=GPIO4=D2 any interrupt able pin (Wemos) // 2=GPIO2 on ESP-01
/// Geiger tube parameters
#define TUBE_NAME "j305"
#define TUBE_FACTOR 0.00812
/// OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
/// Misc
#define DEBUG 1

const char* myhostname = "esp01geiger";
const char* ssid = APSSID;
const char* key =  APKEY;
/// register interrupt handler
void ICACHE_RAM_ATTR tube_impulse();

AsyncWebServer server(8080);
AsyncWebSocket ws("/ws");

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//////////////////////////////////////////////
/// Counter vars

#define LOG_PERIOD 15000     // Logging period in milliseconds, recommended value 15000-60000.
#define MAX_PERIOD 60000     // Maximum logging period

volatile unsigned long counts;  // variable for GM Tube events
volatile bool sig;              // flag that at least one impulse occured interrupt
signed long cpm;              // variable for CPM
float uSv;
float uSh;
unsigned int collectCounter;
unsigned int collectorMax;
bool collectorFull = true;
unsigned int multiplier;        // variable for calculation CPM in this sketch
unsigned long previousMillis;   // variable for time measurement
float collector[60*4];


//////////////////////////////////////////////
// Display part

void setupDisplay(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  } else {
    Serial.println(F("SSD1306 allocated"));
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("initializing");
  display.display();
}

void updateDisplay(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(WiFi.localIP());
  display.println(":8080");
  display.setCursor(0, 10);
  display.setTextSize(2);
  display.print("CPM:");
  display.println(cpm);
  display.print("uSv:");
  display.println(uSv);
  display.print("uSh:");
  display.println(uSh);
  display.display();
}


//////////////////////////////////////////////
// WiFi Manager part

void setupWiFiManager() {
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect(APSSID, APKEY);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  display.clearDisplay();
  display.println("No WiFi found");
  display.print("AP:");
  display.println(myWiFiManager->getConfigPortalSSID());
  display.print("Key:");
  display.println(key);
  display.print("IP:");
  display.println(WiFi.softAPIP());
  display.display();
}


//////////////////////////////////////////////
// Geiger Counter part

void tube_impulse(){            // procedure for capturing events from Geiger Kit
  counts++;
  sig = true;
}

void setupGeiger() {
  // setup interrupt for falling edge on receier pin
  sig = false;
  counts = 0;
  cpm = -1;
  uSv = -1;
  uSh = -1;
  collectCounter = 0;
  collectorFull = false;
  collectorMax = 0;

  multiplier = MAX_PERIOD / LOG_PERIOD;     // calculating multiplier, depend on your log period    
  pinMode(RECEIVER_PIN, INPUT);             // set pin as input for capturing GM Tube events
  attachInterrupt(digitalPinToInterrupt(RECEIVER_PIN), tube_impulse, RISING);  // define external interrupts  
}

void loopGeiger() {
  unsigned long currentMillis = millis();

#ifdef DEBUG
  if (sig) {
    Serial.println("tick!");
    sig = false;
  }
#endif

  if(currentMillis - previousMillis > LOG_PERIOD) {
    previousMillis = currentMillis;
    cpm = counts * multiplier;
    counts = 0;
    uSv = TUBE_FACTOR * cpm;

    Serial.print("collectCounter: ");
    Serial.println(collectCounter);

    collector[collectCounter] = uSv;
    
    if (collectCounter >= 60 * multiplier) {
      collectCounter = 0;
      collectorFull = true;
    }

    if (collectorFull) {
      collectorMax = 60 * multiplier;
    }
    else {
      collectorMax = collectCounter;
    }

    Serial.print("collectorMax: ");
    Serial.println(collectorMax);

    float sum = 0;
    for (int i=0; i<=collectorMax; i++) {
      sum=sum + collector[i];
      Serial.print("collector[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(collector[i]);      

    }
    Serial.print("Sum: ");
    Serial.println(sum);
    uSh = sum / (collectorMax+1);

    notifyClients();
    updateDisplay();

    collectCounter++;
    
#ifdef DEBUG
    Serial.print(cpm); Serial.println(" cpm");
    Serial.print(uSv); Serial.println(" uSv");
    Serial.print(uSh); Serial.println(" uSh");
#endif
  }
}


//////////////////////////////////////////////
// Web part

void notifyClients() {
  ws.textAll("{\n\t\"cpm\": \"" + String(cpm) + "\",\n\t\"uSv\": \"" + String(uSv) + "\",\n\t\"uSh\": \"" + String(uSh) +"\"\n}");
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.println("Data received: ");
    Serial.println((char*)data);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        //handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  if(var == "CPM"){
    return String(cpm);
  }
  else if(var == "USV"){
    return String(uSv);
  }
  else if(var == "USH"){
    return String(uSh);
  }
  else {
    Serial.println("template: Unknown variable");
    return String("Unknown variable");
  }
}


//////////////////////////////////////////////
/// OTA part

void setupOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(myhostname);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    SPIFFS.end();
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

//////////////////////////////////////////////
/// Main part

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
     
  setupDisplay();
  setupWiFiManager();
  
  setupOTA();
  setupGeiger();
  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.begin();
  updateDisplay();
}

void loop() {
  loopGeiger();
  ws.cleanupClients();
}

