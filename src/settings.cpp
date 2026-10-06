#include "settings.h"

#include <ArduinoJson.h>

#include "defaults.h"
#include "platform.h"

Settings settings;

static const char* kSettingsPath = "/settings.json";

static void applyDefaults() {
  settings.name = AIRCON_DEFAULT_NAME;
  settings.room = AIRCON_DEFAULT_ROOM;
  settings.mqttHost = AIRCON_DEFAULT_MQTT_HOST;
  settings.mqttPort = AIRCON_DEFAULT_MQTT_PORT;
  settings.mqttUser = AIRCON_DEFAULT_MQTT_USER;
  settings.mqttPassword = AIRCON_DEFAULT_MQTT_PASSWORD;
  settings.protocol = AIRCON_DEFAULT_PROTOCOL;
  settings.model = AIRCON_DEFAULT_MODEL;
  settings.fahrenheit = AIRCON_DEFAULT_FAHRENHEIT;
  settings.powerOnMode = AIRCON_DEFAULT_POWER_ON_MODE;
  settings.repeatRemote = false;
  settings.sendDelayMs = 500;
  settings.ignoreWindowMs = 300;
  settings.openPortalOnBoot = false;
  settings.legacyCleanedFor = "";
}

bool settingsLoad() {
  applyDefaults();

  File file = LittleFS.open(kSettingsPath, "r");
  if (!file) {
    Serial.println(F("[settings] none saved yet, using defaults"));
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    Serial.printf("[settings] unreadable (%s), using defaults\n", error.c_str());
    return false;
  }

  settings.name = doc["name"] | settings.name;
  settings.room = doc["room"] | settings.room;
  settings.mqttHost = doc["mqttHost"] | settings.mqttHost;
  settings.mqttPort = doc["mqttPort"] | settings.mqttPort;
  settings.mqttUser = doc["mqttUser"] | settings.mqttUser;
  settings.mqttPassword = doc["mqttPassword"] | settings.mqttPassword;
  settings.protocol = doc["protocol"] | settings.protocol;
  settings.model = doc["model"] | settings.model;
  settings.fahrenheit = doc["fahrenheit"] | settings.fahrenheit;
  settings.powerOnMode = doc["powerOnMode"] | settings.powerOnMode;
  settings.repeatRemote = doc["repeatRemote"] | settings.repeatRemote;
  settings.sendDelayMs = doc["sendDelayMs"] | settings.sendDelayMs;
  settings.ignoreWindowMs = doc["ignoreWindowMs"] | settings.ignoreWindowMs;
  settings.openPortalOnBoot = doc["openPortalOnBoot"] | settings.openPortalOnBoot;
  settings.legacyCleanedFor = doc["legacyCleanedFor"] | settings.legacyCleanedFor;

  if (settings.name.isEmpty()) settings.name = AIRCON_DEFAULT_NAME;
  return true;
}

bool settingsSave() {
  JsonDocument doc;
  doc["name"] = settings.name;
  doc["room"] = settings.room;
  doc["mqttHost"] = settings.mqttHost;
  doc["mqttPort"] = settings.mqttPort;
  doc["mqttUser"] = settings.mqttUser;
  doc["mqttPassword"] = settings.mqttPassword;
  doc["protocol"] = settings.protocol;
  doc["model"] = settings.model;
  doc["fahrenheit"] = settings.fahrenheit;
  doc["powerOnMode"] = settings.powerOnMode;
  doc["repeatRemote"] = settings.repeatRemote;
  doc["sendDelayMs"] = settings.sendDelayMs;
  doc["ignoreWindowMs"] = settings.ignoreWindowMs;
  doc["openPortalOnBoot"] = settings.openPortalOnBoot;
  doc["legacyCleanedFor"] = settings.legacyCleanedFor;

  File file = LittleFS.open(kSettingsPath, "w");
  if (!file) {
    Serial.println(F("[settings] could not open file for writing"));
    return false;
  }
  serializeJson(doc, file);
  file.close();
  return true;
}

void settingsFactoryReset() {
  LittleFS.remove(kSettingsPath);
  LittleFS.remove("/state.json");
  applyDefaults();
}

String slugify(const String& text) {
  String slug;
  slug.reserve(text.length());
  for (size_t i = 0; i < text.length(); i++) {
    char c = tolower(text[i]);
    if (isalnum(c)) {
      slug += c;
    } else if (!slug.isEmpty() && slug[slug.length() - 1] != '_') {
      slug += '_';
    }
  }
  while (slug.endsWith("_")) slug.remove(slug.length() - 1);
  return slug;
}
