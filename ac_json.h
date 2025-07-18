#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRac.h>
#include <IRutils.h>

void sendWithCors(int code, const String& contentType, const String& message) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, PUT, POST, PATCH, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(code, contentType, message);
}

String acStateToSerializedJsonString(stdAc::state_t acState, bool pretty = false) {
  StaticJsonDocument<1024> doc;

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

bool sendState() {
  Serial.println("Attempting to send AC state to aircon:");
  Serial.println(acStateToSerializedJsonString(ac .next, true));
  bool wasSuccessful = ac.sendAc();
  lastSentTime = millis();
  acState = ac.getState();
  if (wasSuccessful) {
    Serial.println("Sent state to aircon.");
  } else {
    Serial.println("Failed to send state to aircon.");
  }
  return wasSuccessful;
}

bool sendSerializedJsonAcState(String input) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print("Failed deserializing JSON. Returned: ");
    Serial.println(error.c_str());
    sendWithCors(400, "application/json", "{\"ok\": false, \"error\": \"Invalid JSON\"}");
    return false;
  }

  ac.next.protocol = strToDecodeType((doc["protocol"] | typeToString(acState.protocol)).c_str());
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

  return sendState();
}

String configToJsonString() {
  JsonDocument doc;
  doc["echo"] = echo;
  doc["ignoreWindow"] = ignoreWindow;
  
  String output;

  doc.shrinkToFit();
  serializeJson(doc, output);

  return output; 
}

bool setSerializedJsonConfig(String input) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print("Failed deserializing JSON. Returned: ");
    Serial.println(error.c_str());
    sendWithCors(400, "application/json", "{\"ok\": false, \"error\": \"Invalid JSON\"}");
    return false;
  }

  echo = doc["echo"] | echo;
  ignoreWindow = doc["ignoreWindow"] | ignoreWindow;

  return true;
}
