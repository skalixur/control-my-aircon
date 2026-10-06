// Compile-time defaults. Override any of these in user_config.h (see user_config.example.h).
#pragma once

#if __has_include("../user_config.h")
#include "../user_config.h"
#endif

#define AIRCON_VERSION "2.0.0"
#define AIRCON_PROJECT_URL "https://github.com/skalixur/control-my-aircon"

// === Pins ===
#ifndef AIRCON_IR_LED_PIN
#if defined(ESP8266)
#define AIRCON_IR_LED_PIN D1  // IR LED (or transistor base/gate)
#else
#define AIRCON_IR_LED_PIN 4
#endif
#endif

#ifndef AIRCON_IR_RECEIVER_PIN
#if defined(ESP8266)
#define AIRCON_IR_RECEIVER_PIN D2  // VS1838B / TSOP data pin
#else
#define AIRCON_IR_RECEIVER_PIN 14
#endif
#endif

// Status LED. Set to -1 to disable.
#ifndef AIRCON_STATUS_LED_PIN
#if defined(ESP8266)
#define AIRCON_STATUS_LED_PIN LED_BUILTIN
#else
#define AIRCON_STATUS_LED_PIN -1
#endif
#endif

#ifndef AIRCON_STATUS_LED_INVERTED
#if defined(ESP8266)
#define AIRCON_STATUS_LED_INVERTED true  // NodeMCU's LED is active-low
#else
#define AIRCON_STATUS_LED_INVERTED false
#endif
#endif

// The FLASH/BOOT button. Tap: learn from remote. Hold 5 s: factory reset. Set to -1 to disable.
#ifndef AIRCON_BUTTON_PIN
#define AIRCON_BUTTON_PIN 0
#endif

// === First-boot defaults (all of these can be changed later from the setup portal or Home Assistant) ===
#ifndef AIRCON_DEFAULT_NAME
#define AIRCON_DEFAULT_NAME "Aircon"
#endif

#ifndef AIRCON_DEFAULT_ROOM
#define AIRCON_DEFAULT_ROOM ""
#endif

// Leave empty to find the broker automatically (mDNS, then the Home Assistant host).
#ifndef AIRCON_DEFAULT_MQTT_HOST
#define AIRCON_DEFAULT_MQTT_HOST ""
#endif

#ifndef AIRCON_DEFAULT_MQTT_PORT
#define AIRCON_DEFAULT_MQTT_PORT 1883
#endif

#ifndef AIRCON_DEFAULT_MQTT_USER
#define AIRCON_DEFAULT_MQTT_USER ""
#endif

#ifndef AIRCON_DEFAULT_MQTT_PASSWORD
#define AIRCON_DEFAULT_MQTT_PASSWORD ""
#endif

// Protocol name as printed by IRremoteESP8266 (e.g. "COOLIX", "DAIKIN"). Leave empty to learn it from your remote.
#ifndef AIRCON_DEFAULT_PROTOCOL
#define AIRCON_DEFAULT_PROTOCOL ""
#endif

#ifndef AIRCON_DEFAULT_MODEL
#define AIRCON_DEFAULT_MODEL -1
#endif

// What "turn on" resumes in: "Last Used", "Cool", "Heat", "Auto", "Dry" or "Fan". Changeable from Home Assistant.
#ifndef AIRCON_DEFAULT_POWER_ON_MODE
#define AIRCON_DEFAULT_POWER_ON_MODE "Last Used"
#endif

#ifndef AIRCON_DEFAULT_FAHRENHEIT
#define AIRCON_DEFAULT_FAHRENHEIT false
#endif

// Optional: pre-fill WiFi so the setup portal is skipped on first boot.
#ifndef AIRCON_WIFI_SSID
#define AIRCON_WIFI_SSID ""
#endif

#ifndef AIRCON_WIFI_PASSWORD
#define AIRCON_WIFI_PASSWORD ""
#endif

// Optional: protects OTA uploads (Arduino IDE network port and http://<device>/update). Username is "admin".
#ifndef AIRCON_OTA_PASSWORD
#define AIRCON_OTA_PASSWORD ""
#endif

// === Temperature limits ===
#ifndef AIRCON_MIN_TEMP_C
#define AIRCON_MIN_TEMP_C 16
#endif
#ifndef AIRCON_MAX_TEMP_C
#define AIRCON_MAX_TEMP_C 30
#endif
#ifndef AIRCON_MIN_TEMP_F
#define AIRCON_MIN_TEMP_F 60
#endif
#ifndef AIRCON_MAX_TEMP_F
#define AIRCON_MAX_TEMP_F 86
#endif
