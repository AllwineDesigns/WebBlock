#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"

#define SERIAL_BAUDRATE 115200
#define PIN1_PIN 3
#define PIN2_PIN 2
#define PIN3_PIN 0

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"__EOF__(
<!DOCTYPE HTML><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Web Block</title>
<style>
#pin1Value,#pin2Value,#pin3Value {
  width: 70%;
}
</style>
</head>
<body>
<h1>Web Block</h1>
<p>Pin 1 <input type="checkbox" id="pin1On"/> <input type="range" id="pin1Value" min="0" max="255" value="0" step="1" class="slider"> <span id="pin1Text"></span></p>
<p>Pin 2 <input type="checkbox" id="pin2On"/> <input type="range" id="pin2Value" min="0" max="255" value="0" step="1" class="slider"> <span id="pin2Text"></span></p>
<p>Pin 3 <input type="checkbox" id="pin3On"/> <input type="range" id="pin3Value" min="0" max="255" value="0" step="1" class="slider"> <span id="pin3Text"></span></p>
<script>
const pin1OnEl = document.getElementById("pin1On");
const pin2OnEl = document.getElementById("pin2On");
const pin3OnEl = document.getElementById("pin3On");
const pin1ValueEl = document.getElementById("pin1Value");
const pin2ValueEl = document.getElementById("pin2Value");
const pin3ValueEl = document.getElementById("pin3Value");
const pin1TextEl = document.getElementById("pin1Text");
const pin2TextEl = document.getElementById("pin2Text");
const pin3TextEl = document.getElementById("pin3Text");
const pin1OnChange = () => {
  fetch("/update?pin1On=" + pin1OnEl.checked); 
};
const pin2OnChange = () => {
  fetch("/update?pin2On=" + pin2OnEl.checked); 
};
const pin3OnChange = () => {
  fetch("/update?pin3On=" + pin3OnEl.checked); 
};
const pin1ValueChange = () => {
  pin1TextEl.innerHTML = pin1ValueEl.value;
  fetch("/update?pin1Value=" + pin1ValueEl.value); 
};
const pin2ValueChange = () => {
  pin2TextEl.innerHTML = pin2ValueEl.value;
  fetch("/update?pin2Value=" + pin2ValueEl.value); 
};
const pin3ValueChange = () => {
  pin3TextEl.innerHTML = pin3ValueEl.value;
  fetch("/update?pin3Value=" + pin3ValueEl.value); 
};
pin1OnEl.addEventListener("change",pin1OnChange);
pin2OnEl.addEventListener("change",pin2OnChange);
pin3OnEl.addEventListener("change",pin3OnChange);
pin1ValueEl.addEventListener("change",pin1ValueChange);
pin2ValueEl.addEventListener("change",pin2ValueChange);
pin3ValueEl.addEventListener("change",pin3ValueChange);
fetch("/get").then((res) => res.json()).then((obj) => {
  pin1OnEl.checked = obj.pin1On;
  pin2OnEl.checked = obj.pin2On;
  pin3OnEl.checked = obj.pin3On;

  pin1ValueEl.value = obj.pin1Value;
  pin2ValueEl.value = obj.pin2Value;
  pin3ValueEl.value = obj.pin3Value;

  pin1TextEl.innerHTML = obj.pin1Value;
  pin2TextEl.innerHTML = obj.pin2Value;
  pin3TextEl.innerHTML = obj.pin3Value;
});
</script>
</body>
</html>
)__EOF__";

int pin1On = 0;
int pin2On = 0;
int pin3On = 0;

long int pin1Value = 255;
long int pin2Value = 255;
long int pin3Value = 255;

long int strToInt(const char* str) {
  long int v = strtol(str, NULL, 10);
  if(v < 0) {
    v = 0;
  } else if(v > 255) {
    v = 255;
  }
  return v;
}

void setup() {
  // set RX pin to be GPIO
  pinMode(3, FUNCTION_3);

  pinMode(PIN1_PIN, OUTPUT);
  pinMode(PIN2_PIN, OUTPUT);
  pinMode(PIN3_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  boolean res = WiFi.softAP("WebBlock", "12345678");
  // Init serial port and clean garbage
  Serial.begin(SERIAL_BAUDRATE);
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());

  if(!res) {
    Serial.println("Failed to connect.");
  } else {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        const char* name = p->name().c_str();
        const char* value = p->value().c_str();
        if(strcmp(name,"pin1On") == 0) {
          if(strcmp(value,"true")==0) {
            pin1On = 1;
          } else {
            pin1On = 0;
          }
        } else if(strcmp(name, "pin2On") == 0) {
          if(strcmp(value,"true")==0) {
            pin2On = 1;
          } else {
            pin2On = 0;
          }
        } else if(strcmp(name, "pin3On") == 0) {
          if(strcmp(value,"true")==0) {
            pin3On = 1;
          } else {
            pin3On = 0;
          }
        } else if(strcmp(name, "pin1Value") == 0) {
          pin1Value = strToInt(value);
        } else if(strcmp(name, "pin2Value") == 0) {
          pin2Value = strToInt(value);
        } else if(strcmp(name, "pin3Value") == 0) {
          pin3Value = strToInt(value);
        }
      }
      analogWrite(PIN1_PIN, pin1On*pin1Value);
      analogWrite(PIN2_PIN, pin2On*pin2Value);
      analogWrite(PIN3_PIN, pin3On*pin3Value);
      request->send(200, "text/plain", "OK");
    });
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      DynamicJsonDocument doc(256);
      doc["pin1On"] = pin1On;
      doc["pin2On"] = pin2On;
      doc["pin3On"] = pin3On;
      doc["pin1Value"] = pin1Value;
      doc["pin2Value"] = pin2Value;
      doc["pin3Value"] = pin3Value;
      serializeJson(doc, *response);
      request->send(response);
    });
    server.begin();
  }
}

void loop() {

  // This is a sample code to output free heap every 5 seconds
  // This is a cheap way to detect memory leaks
  static unsigned long last = millis();
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
  }

}
