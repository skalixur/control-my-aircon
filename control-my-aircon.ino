// control-my-aircon: an IR aircon controller that shows up in Home Assistant by itself.
//
// First boot: join the "Aircon Setup" WiFi network from your phone, pick your WiFi, enter your MQTT login,
// then press any button on your aircon remote while pointing it at the device. That's it.
//
// Everything lives in src/. Optional compile-time overrides go in user_config.h (see user_config.example.h).

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>

#include "src/aircon.h"
#include "src/defaults.h"
#include "src/home_assistant.h"
#include "src/platform.h"
#include "src/settings.h"
#include "src/status_led.h"
#include "src/system.h"
#include "src/web.h"

const uint32_t kPortalTimeoutSeconds = 300;
const uint32_t kOutagePortalTimeoutSeconds = 120;  // saved WiFi didn't connect, probably the router is still booting
const uint32_t kFactoryResetHoldMs = 5000;

String deviceId;  // "aircon_a1b2c3"
String hostname;  // "aircon-a1b2c3" -> http://aircon-a1b2c3.local

// Setup portal fields. They live for the whole program because WiFiManager keeps pointers to them.
WiFiManagerParameter introParam("<p style='opacity:.75'>Leave <b>MQTT server</b> blank to find Home Assistant's "
                                "broker automatically. Use the same login you gave the Mosquitto add-on (any Home "
                                "Assistant user works).</p>");
WiFiManagerParameter nameParam("name", "Name", "", 40);
WiFiManagerParameter roomParam("room", "Room (optional)", "", 40);
WiFiManagerParameter mqttHostParam("mqtt_host", "MQTT server IP (blank = automatic)", "", 64);
WiFiManagerParameter mqttPortParam("mqtt_port", "MQTT port", "", 6, "inputmode='numeric'");
WiFiManagerParameter mqttUserParam("mqtt_user", "MQTT username", "", 64);
WiFiManagerParameter mqttPasswordParam("mqtt_password", "MQTT password (blank = keep current)", "", 64, "type='password'");
WiFiManagerParameter protocolParam("protocol", "Aircon protocol, e.g. COOLIX (optional, otherwise learned from the remote)",
                                   "", 32);
WiFiManagerParameter modelParam("model", "IR model (optional)", "", 6, "inputmode='numeric'");
bool portalSaved = false;
bool wifiSaveRequested = false;  // the WiFi page was submitted (vs. only the settings page)

// WiFiManager pastes values into value='...' unescaped; an apostrophe ("Kid's Room") would truncate the field.
String htmlEscape(const String& text) {
  String escaped;
  escaped.reserve(text.length());
  for (size_t i = 0; i < text.length(); i++) {
    switch (text[i]) {
      case '&': escaped += F("&amp;"); break;
      case '\'': escaped += F("&#39;"); break;
      case '"': escaped += F("&quot;"); break;
      case '<': escaped += F("&lt;"); break;
      case '>': escaped += F("&gt;"); break;
      default: escaped += text[i];
    }
  }
  return escaped;
}

void setParam(WiFiManagerParameter& param, const String& value) {
  param.setValue(htmlEscape(value).c_str(), param.getValueLength());
}

void savePortalParams() {
  settings.name = nameParam.getValue();
  settings.name.trim();
  if (settings.name.isEmpty()) settings.name = AIRCON_DEFAULT_NAME;
  settings.room = roomParam.getValue();
  settings.room.trim();
  settings.mqttHost = mqttHostParam.getValue();
  settings.mqttHost.trim();
  int port = atoi(mqttPortParam.getValue());
  settings.mqttPort = port > 0 && port < 65536 ? port : 1883;
  settings.mqttUser = mqttUserParam.getValue();
  String password = mqttPasswordParam.getValue();
  if (!password.isEmpty()) settings.mqttPassword = password;  // the field is never pre-filled
  settingsSave();

  // aircon::apply validates the name, saves it and updates the live IR state.
  String protocol = protocolParam.getValue();
  protocol.trim();
  if (!protocol.isEmpty() && aircon::apply("protocol", protocol.c_str()) != aircon::Result::kApplied) {
    Serial.printf("[setup] \"%s\" isn't a protocol IRremoteESP8266 can send; learn it from the remote instead\n",
                  protocol.c_str());
  }
  String model = modelParam.getValue();
  model.trim();
  if (!model.isEmpty()) aircon::apply("model", model.c_str());

  portalSaved = true;
  Serial.println(F("[setup] settings saved"));
}

void connectWifi() {
  WiFiManager wm;
  wm.setDebugOutput(false);
  wm.setHostname(hostname);
  wm.setTitle("Aircon setup");
  wm.setDarkMode(true);
  wm.setConnectTimeout(30);
  wm.setConfigPortalBlocking(false);
  std::vector<const char*> menu = { "wifi", "param", "info", "exit" };
  wm.setMenu(menu);

  setParam(nameParam, settings.name);
  setParam(roomParam, settings.room);
  setParam(mqttHostParam, settings.mqttHost);
  setParam(mqttPortParam, String(settings.mqttPort));
  setParam(mqttUserParam, settings.mqttUser);
  setParam(protocolParam, settings.protocol);
  setParam(modelParam, settings.model >= 0 ? String(settings.model) : String());
  wm.addParameter(&introParam);
  wm.addParameter(&nameParam);
  wm.addParameter(&roomParam);
  wm.addParameter(&mqttHostParam);
  wm.addParameter(&mqttPortParam);
  wm.addParameter(&mqttUserParam);
  wm.addParameter(&mqttPasswordParam);
  wm.addParameter(&protocolParam);
  wm.addParameter(&modelParam);
  wm.setSaveParamsCallback(savePortalParams);
  // The WiFi page with an empty SSID only saves settings; anything else is a WiFi change WiFiManager will connect to.
  wm.setPreSaveConfigCallback([&wm] {
    wifiSaveRequested = !wm.server->arg("s").isEmpty() || !wm.server->arg("p").isEmpty();
  });

  // Credentials compiled in through user_config.h skip the portal entirely.
  if (strlen(AIRCON_WIFI_SSID) > 0 && !wm.getWiFiIsSaved()) WiFi.begin(AIRCON_WIFI_SSID, AIRCON_WIFI_PASSWORD);

  String apName = "Aircon Setup " + deviceId.substring(deviceId.length() - 4);
  bool forcePortal = settings.openPortalOnBoot;
  if (forcePortal) {
    // A portal requested from Home Assistant opens once. On first boot it keeps reopening until settings are saved.
    settings.openPortalOnBoot = false;
    if (LittleFS.exists("/settings.json")) settingsSave();
  }

  bool hadWifi = wm.getWiFiIsSaved();
  wm.setConfigPortalTimeout(hadWifi && !forcePortal ? kOutagePortalTimeoutSeconds : kPortalTimeoutSeconds);

  bool connected = forcePortal ? wm.startConfigPortal(apName.c_str()) : wm.autoConnect(apName.c_str());
  if (connected && !forcePortal) return;

  Serial.printf("[setup] join the WiFi network \"%s\" and open http://192.168.4.1\n", apName.c_str());
  statusLed::setPattern(statusLed::Pattern::kSetup);
  while (wm.getConfigPortalActive()) {
    if (wm.process()) break;  // new WiFi saved and connected
    if (portalSaved && hadWifi && !wifiSaveRequested) {
      // Only the name/MQTT/protocol settings changed. WiFiManager switched the station off while the portal was
      // open, so the cleanest way back onto the saved WiFi is a restart.
      Serial.println(F("[setup] restarting with the new settings"));
      delay(1000);  // let the "saved" page reach the phone
      ESP.restart();
    }
    aircon::loop();  // the remote can be learned while you're still in the portal
    statusLed::loop();
    delay(5);
  }

  if (WiFi.status() != WL_CONNECTED && hadWifi) {
    WiFi.mode(WIFI_STA);
    WiFi.begin();  // saved credentials
    WiFi.waitForConnectResult(20000);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[setup] no WiFi; restarting to try again"));
    delay(500);
    ESP.restart();
  }
}

void setupOta() {
  ArduinoOTA.setHostname(hostname.c_str());
  if (strlen(AIRCON_OTA_PASSWORD) > 0) ArduinoOTA.setPassword(AIRCON_OTA_PASSWORD);
  ArduinoOTA.onStart([] {
    Serial.println(F("[ota] update starting"));
    aircon::flush();
    homeAssistant::goOffline();
  });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("[ota] failed (%u)\n", error); });
  ArduinoOTA.begin();
}

// Tap FLASH/BOOT: learn from remote. Hold 5 s: factory reset.
void handleButton() {
  if (AIRCON_BUTTON_PIN < 0) return;
  static uint32_t pressedAt = 0;
  static bool wasPressed = false;
  bool pressed = digitalRead(AIRCON_BUTTON_PIN) == LOW;
  uint32_t now = millis();

  if (pressed && !wasPressed) pressedAt = now;
  if (pressed && now - pressedAt >= kFactoryResetHoldMs) statusLed::setPattern(statusLed::Pattern::kResetArmed);

  if (!pressed && wasPressed) {
    uint32_t held = now - pressedAt;
    if (held >= kFactoryResetHoldMs) {
      Serial.println(F("[system] factory reset"));
      settingsFactoryReset();
      WiFiManager wm;
      wm.resetSettings();
      delay(200);
      ESP.restart();
    } else if (held >= 50) {
      aircon::startLearning();
    }
  }
  wasPressed = pressed;
}

void updateStatusLed() {
  if (AIRCON_BUTTON_PIN >= 0 && digitalRead(AIRCON_BUTTON_PIN) == LOW) return;  // handleButton owns it
  if (aircon::isLearning()) {
    statusLed::setPattern(statusLed::Pattern::kLearning);
  } else if (WiFi.status() != WL_CONNECTED || !homeAssistant::connected()) {
    statusLed::setPattern(statusLed::Pattern::kConnecting);
  } else {
    statusLed::setPattern(statusLed::Pattern::kOff);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println(F("control-my-aircon " AIRCON_VERSION));

  statusLed::begin();
  if (AIRCON_BUTTON_PIN >= 0) pinMode(AIRCON_BUTTON_PIN, INPUT_PULLUP);
  bool fsReady = fsBegin();
  if (!fsReady) Serial.println(F("[fs] LittleFS unavailable; settings won't survive a restart"));
  // First boot of this firmware: show the portal even if the chip remembers WiFi from an older firmware,
  // so the MQTT login gets asked for. Compiled-in credentials (user_config.h) skip it.
  if (!settingsLoad() && fsReady && strlen(AIRCON_WIFI_SSID) == 0) settings.openPortalOnBoot = true;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%02x%02x%02x", mac[3], mac[4], mac[5]);
  deviceId = String("aircon_") + suffix;
  hostname = String("aircon-") + suffix;

  aircon::begin();
  connectWifi();

  Serial.printf("[wifi] connected to %s as %s (http://%s.local)\n", WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str(), hostname.c_str());

  setupOta();
  homeAssistant::begin(deviceId);
  web::begin(deviceId);
}

void loop() {
  ArduinoOTA.handle();
  mdnsLoop();
  web::loop();
  homeAssistant::loop();
  aircon::loop();
  systemControl::loop();
  handleButton();
  updateStatusLed();
  statusLed::loop();
}
