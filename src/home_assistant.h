// MQTT connection and Home Assistant discovery.
#pragma once

#include <Arduino.h>

namespace homeAssistant {

// deviceId is unique per chip, e.g. "aircon_a1b2c3".
void begin(const String& deviceId);
void loop();
bool connected();
// Where the broker was found, e.g. "192.168.1.10:1883", or "" if not found yet.
String brokerAddress();

void publishState();
void publishRemoteEvent(const char* eventType);
void publishDiscovery();

// Marks the device unavailable and disconnects cleanly (before a restart).
void goOffline();

}  // namespace homeAssistant
