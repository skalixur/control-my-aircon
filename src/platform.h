// Thin layer over the differences between the ESP8266 and ESP32 Arduino cores.
#pragma once

#include <Arduino.h>
#include <LittleFS.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
using WebServerType = ESP8266WebServer;
using UpdateServerType = ESP8266HTTPUpdateServer;
#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <ESPmDNS.h>
using WebServerType = WebServer;
using UpdateServerType = HTTPUpdateServer;
#else
#error "control-my-aircon supports ESP8266 and ESP32 boards only."
#endif

inline bool fsBegin() {
#if defined(ESP8266)
  return LittleFS.begin();  // formats automatically on first use
#else
  return LittleFS.begin(true);
#endif
}

// Finds an MQTT broker on the LAN: an advertised _mqtt._tcp service first, then the Home Assistant host
// (the Mosquitto add-on runs there on 1883). Returns false if nothing answered.
inline bool discoverBroker(IPAddress& ip, uint16_t& port) {
  struct Candidate {
    const char* service;
    uint16_t forcedPort;
  };
  const Candidate candidates[] = { { "mqtt", 0 }, { "home-assistant", 1883 } };

  for (const Candidate& c : candidates) {
    int found = MDNS.queryService(c.service, "tcp");
    for (int i = 0; i < found; i++) {
#if defined(ESP8266)
      IPAddress candidateIp = MDNS.IP(i);
#else
      IPAddress candidateIp = MDNS.address(i);
#endif
      if (candidateIp == IPAddress(0, 0, 0, 0)) continue;
      ip = candidateIp;
      port = c.forcedPort ? c.forcedPort : MDNS.port(i);
      return true;
    }
  }
  return false;
}

inline void mdnsLoop() {
#if defined(ESP8266)
  MDNS.update();
#endif
}
