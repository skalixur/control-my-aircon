// Persistent device settings, stored as JSON on LittleFS.
#pragma once

#include <Arduino.h>

struct Settings {
  String name;
  String room;

  String mqttHost;  // empty = discover automatically
  uint16_t mqttPort;
  String mqttUser;
  String mqttPassword;

  String protocol;  // IRremoteESP8266 protocol name; empty = not learned yet
  int16_t model;
  bool fahrenheit;
  String powerOnMode;  // a name from kPowerOnModes: what "turn on" resumes in

  bool repeatRemote;      // re-transmit what the physical remote sent
  uint16_t sendDelayMs;   // debounce before transmitting, so rapid changes become one IR burst
  uint16_t ignoreWindowMs;  // ignore IR received right after we transmit (our own echo)

  bool openPortalOnBoot;
  String legacyCleanedFor;  // room whose v1 MQTT topics have already been removed
};

extern Settings settings;

// Returns false if nothing was saved yet (first boot).
bool settingsLoad();
bool settingsSave();
void settingsFactoryReset();

// "Living Room" -> "living_room"
String slugify(const String& text);
