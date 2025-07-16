#include <Arduino.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRac.h>
#include <IRutils.h>
#include "config.h"

unsigned long lastSentTime = 0;

WiFiEventHandler gotIpEventHandler;
ESP8266WebServer server(port);
IRac ac(kIrLed);
IRrecv irrecv(kIrReceiver, kCaptureBufferSize, kTimeout, true);
decode_results results;
stdAc::state_t acState;

void sendWithCors(int code, const String& contentType, const String& message) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, PUT, POST, PATCH, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(code, contentType, message);
}

String acStateToSerializedJsonString(stdAc::state_t acState, bool pretty = false) {
  JsonDocument<1024> doc;

  doc["protocol"] = acState.protocol;
  doc["model"] = acState.model;
  doc["power"] = acState.power;
  doc["mode"] = ac.opmodeToString(acState.mode);
  doc["degrees"] = acState.degrees;
  doc["celsius"] = acState.celsius;
  doc["fanspeed"] = ac.fanspeedToString(acState.fanspeed);
  doc["swingv"] = ac.swingvToString(acState.swingv);
  doc["swingh"] = ac.swinghToString(acState.swingh);
  doc["quiet"] = acState.quiet;
  doc["turbo"] = acState.turbo;
  doc["econo"] = acState.econo;
  doc["light"] = acState.light;
  doc["filter"] = acState.filter;
  doc["clean"] = acState.clean;
  doc["beep"] = acState.beep;
  doc["sleep"] = acState.sleep;
  doc["clock"] = acState.clock;
  doc["command"] = ac.commandTypeToString(acState.command);
  doc["iFeel"] = acState.iFeel;
  doc["sensorTemperature"] = acState.sensorTemperature;

  String output;

  if (pretty) {
    serializeJsonPretty(doc, output);
  } else {
    serializeJson(doc, output);
  }

  return output;
}

void serializedJsonToAcState(String input) {
  JsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print("Failed deserializing JSON. Returned: ");
    Serial.println(error.c_str());
    sendWithCors(400, "application/json", "{\"ok\": false, \"error\": \"Invalid JSON\"}");
    return;
  }

  ac.next.model = doc["model"] | acState.model;
  ac.next.power = doc["power"] | acState.power;
  ac.next.mode = ac.strToOpmode((doc["mode"] | ac.opmodeToString(acState.mode)).c_str());
  ac.next.degrees = doc["degrees"] | acState.degrees;
  ac.next.celsius = doc["celsius"] | acState.celsius;
  ac.next.fanspeed = ac.strToFanspeed((doc["fanspeed"] | ac.fanspeedToString(acState.fanspeed)).c_str());
  ac.next.swingv = ac.strToSwingV((doc["swingv"] | ac.swingvToString(acState.swingv)).c_str());
  ac.next.swingh = ac.strToSwingH((doc["swingh"] | ac.swinghToString(acState.swingh)).c_str());
  ac.next.quiet = doc["quiet"] | acState.quiet;
  ac.next.turbo = doc["turbo"] | acState.turbo;
  ac.next.econo = doc["econo"] | acState.econo;
  ac.next.light = doc["light"] | acState.light;
  ac.next.filter = doc["filter"] | acState.filter;
  ac.next.clean = doc["clean"] | acState.clean;
  ac.next.beep = doc["beep"] | acState.beep;
  ac.next.sleep = doc["sleep"] | acState.sleep;
  ac.next.clock = doc["clock"] | acState.clock;
  ac.next.command = ac.strToCommandType((doc["command"] | ac.commandTypeToString(acState.command)).c_str());
  ac.next.iFeel = doc["iFeel"] | acState.iFeel;
  ac.next.sensorTemperature = doc["sensorTemperature"] | acState.sensorTemperature;

  sendState();
}

void handleRoot() {
  sendWithCors(200, "text/plain", rootText);
}

void handleGetAlive() {
  sendWithCors(200, "text/plain", "I am alive!");
}

void handleGetState() {
  String state = acStateToSerializedJsonString(acState);
  sendWithCors(200, "application/json", state);
}

void handlePutState() {
  String postBody = server.arg("plain");
  serializedJsonToAcState(postBody);
  sendWithCors(200, "application/json", "{ \"ok\": true }");
}

void restart() {
  sendWithCors(200, "text/plain", "Restarting...");
  delay(100);
  ESP.restart();
}

void sendState() {
  ac.sendAc();
  lastSentTime = millis();
  acState = ac.getState();
  Serial.println("Sent state to aircon:");
  Serial.println(acStateToSerializedJsonString(acState, true));
}

void handleGotIp(const WiFiEventStationModeGotIP& event) {
  Serial.print("Successfully connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: ");
  Serial.println(WiFi.hostname());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void setup() {
  Serial.begin(kBaudRate);

  // Device setup
  irrecv.setTolerance(kTolerancePercentage);
  irrecv.enableIRIn();

  ac.next.protocol = protocol;  // Set a protocol to use.

  acState = ac.getState();

  // Network setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  gotIpEventHandler = WiFi.onStationModeGotIP(&handleGotIp);

  // Ensure WiFi is connected before anything else
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (MDNS.begin(mDnsName)) {
    Serial.println("MDNS responder started sucessfully");
  }

  Serial.println();

  // Server setup
  server.on("/", handleRoot);
  server.on("/alive", HTTP_GET, handleGetAlive);
  server.on("/state", HTTP_GET, handleGetState);
  server.on("/state", HTTP_PUT, handlePutState);
  server.on("/send", HTTP_POST, sendState);
  server.on("/restart", restart);

  server.begin();
}

void loop() {

  server.handleClient();
  MDNS.update();

  if (irrecv.decode(&results) && millis() - lastSentTime >= ignoreWindow) {
      IRAcUtils::decodeToState(&results, &(ac.next));
      if (echo) sendState();
      else acState = ac.getState();

      Serial.println("Command received, new internal AC state: ");
      Serial.println(acStateToSerializedJsonString(acState, true));
  }
}
