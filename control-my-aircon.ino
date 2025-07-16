#include <Arduino.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRac.h>
#include <IRutils.h>
#include "config.h"

const uint16_t kIrLed = D1;

WiFiEventHandler gotIpEventHandler;
ESP8266WebServer server(port);
IRac ac(kIrLed);

String acStateToSerializedJson(stdAc::state_t acState) {
  JsonDocument doc;

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
  serializeJson(doc, output);

  return output;
}

void handleGetState() {
  stdAc::state_t acState = ac.getState();
  String state = acStateToSerializedJson(acState);
  server.send(200, "text/plain", state);
}

void handleRoot() {
  server.send(200, "text/plain", rootText);
}

void handleGetAlive() {
  server.send(200, "text/plain", "I am alive!");
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
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  gotIpEventHandler = WiFi.onStationModeGotIP(&handleGotIp);

  ac.next.protocol = protocol;
  ac.next.model = 1;
  ac.next.mode = stdAc::opmode_t::kCool;
  ac.next.celsius = true;
  ac.next.degrees = 25;
  ac.next.fanspeed = stdAc::fanspeed_t::kMedium;
  ac.next.swingv = stdAc::swingv_t::kOff;
  ac.next.swingh = stdAc::swingh_t::kOff;
  ac.next.light = false;
  ac.next.beep = false;
  ac.next.econo = false;
  ac.next.filter = false;
  ac.next.turbo = false;
  ac.next.quiet = false;
  ac.next.sleep = -1;
  ac.next.clean = false;
  ac.next.clock = -1;
  ac.next.power = false;

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

  server.on("/", handleRoot);
  server.on("/alive", HTTP_GET, handleGetAlive);
  server.on("/state", HTTP_GET, handleGetState);

  server.begin();
}

void loop() {

  if (!ac.isProtocolSupported(protocol)) {
    Serial.println("Selected protocol is not supported by library!");
    delay(10000);
    return;
  }

  server.handleClient();
  MDNS.update();

  // Serial.println("Protocol " + String(protocol) + " / " + typeToString(protocol) + " is supported.");

  // ac.next.power = true;
  // Serial.println("Turning the aircon on");
  // ac.sendAc();
  // delay(5000);

  // ac.next.power = false;
  // Serial.println("Turning the aircon off");
  // ac.sendAc();
  // delay(5000);

  // Serial.println("Repeating\n");
}
