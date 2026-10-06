#include "home_assistant.h"

#include <ArduinoJson.h>
#include <IRac.h>
#include <IRutils.h>
#include <PubSubClient.h>

#include "aircon.h"
#include "defaults.h"
#include "platform.h"
#include "settings.h"
#include "system.h"

namespace homeAssistant {
namespace {

const char* kStatusTopic = "homeassistant/status";  // Home Assistant's birth/last-will topic
const uint32_t kDiagnosticsIntervalMs = 60000;
const uint32_t kMaxRetryDelayMs = 60000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String id;
String stateTopic;
String availabilityTopic;
String commandPrefix;  // ends with '/'
String eventTopic;
String diagnosticsTopic;

String brokerHost;
IPAddress brokerIp;
uint16_t brokerPort = 0;
bool brokerKnown = false;
uint8_t failedAttempts = 0;

uint32_t nextAttemptAt = 0;
uint32_t retryDelayMs = 2000;
uint32_t lastDiagnosticsAt = 0;
bool statePending = false;
bool discoveryPending = false;
uint32_t discoveryAt = 0;

bool due(uint32_t at) {
  return static_cast<int32_t>(millis() - at) >= 0;
}

// Serializes into one heap buffer and sends it with beginPublish, which bypasses PubSubClient's 1 KB buffer.
// Discovery is several KB.
bool publishJson(const String& topic, const JsonDocument& doc, bool retained) {
  size_t length = measureJson(doc);
  char* buffer = static_cast<char*>(malloc(length + 1));
  if (!buffer) {
    Serial.printf("[mqtt] out of memory publishing %u bytes to %s\n", length, topic.c_str());
    return false;
  }
  serializeJson(doc, buffer, length + 1);
  bool ok = mqtt.beginPublish(topic.c_str(), length, retained) &&
            mqtt.write(reinterpret_cast<uint8_t*>(buffer), length) == length && mqtt.endPublish();
  free(buffer);
  if (!ok) Serial.printf("[mqtt] publish to %s failed\n", topic.c_str());
  return ok;
}

JsonObject addComponent(JsonObject components, const char* key, const char* platform, const char* name) {
  JsonObject c = components[key].to<JsonObject>();
  c["platform"] = platform;
  c["unique_id"] = id + "_" + key;
  if (name) {
    c["name"] = name;
  } else {
    c["name"] = nullptr;  // use the device name, e.g. "Bedroom Aircon"
  }
  c["availability_topic"] = availabilityTopic;
  return c;
}

void bindState(JsonObject c, const char* key) {
  c["state_topic"] = stateTopic;
  c["value_template"] = String("{{ value_json['") + key + "'] }}";
}

void bindCommand(JsonObject c, const char* key) {
  c["command_topic"] = commandPrefix + key;
}

struct SwitchSpec {
  const char* key;
  const char* name;
  const char* icon;
  bool enabledByDefault;
  bool config;
};

const SwitchSpec kSwitches[] = {
  { "quiet", "Quiet Mode", "mdi:fan-chevron-down", true, false },
  { "turbo", "Turbo Mode", "mdi:fan-chevron-up", true, false },
  { "econo", "Eco Mode", "mdi:leaf", true, false },
  { "sleep", "Sleep Mode", "mdi:power-sleep", true, false },
  { "light", "Display Light", "mdi:lightbulb-on-outline", true, false },
  { "filter", "Air Filter", "mdi:air-filter", false, false },
  { "clean", "Self-Clean", "mdi:shimmer", false, false },
  { "beep", "Beep Sounds", "mdi:volume-high", false, false },
  { "repeat_remote", "Repeat Remote Commands", "mdi:repeat", true, true },
};

void addDevice(JsonObject device) {
  device["identifiers"].to<JsonArray>().add(id);
  device["name"] = settings.name;
  device["manufacturer"] = "Control My Aircon";
  device["model"] = "IR Aircon Controller";
  device["sw_version"] = AIRCON_VERSION;
  device["configuration_url"] = "http://" + WiFi.localIP().toString() + "/";
  if (!settings.room.isEmpty()) device["suggested_area"] = settings.room;
  JsonArray connection = device["connections"].to<JsonArray>().add<JsonArray>();
  connection.add("mac");
  connection.add(WiFi.macAddress());
}

bool publishDeviceDiscovery() {
  JsonDocument doc;
  addDevice(doc["device"].to<JsonObject>());

  JsonObject origin = doc["origin"].to<JsonObject>();
  origin["name"] = "control-my-aircon";
  origin["sw_version"] = AIRCON_VERSION;
  origin["support_url"] = AIRCON_PROJECT_URL;

  JsonObject components = doc["components"].to<JsonObject>();

  // The thermostat card.
  JsonObject climate = addComponent(components, "climate", "climate", nullptr);
  JsonArray modes = climate["modes"].to<JsonArray>();
  modes.add("off");
  addNames(modes, kModes);
  addNames(climate["fan_modes"].to<JsonArray>(), kFanSpeeds);
  addNames(climate["swing_modes"].to<JsonArray>(), kSwingV);
  addNames(climate["swing_horizontal_modes"].to<JsonArray>(), kSwingH);
  climate["temperature_unit"] = settings.fahrenheit ? "F" : "C";
  climate["min_temp"] = aircon::minTemp();
  climate["max_temp"] = aircon::maxTemp();
  climate["temp_step"] = 1;
  climate["precision"] = 1.0;
  climate["power_command_topic"] = commandPrefix + "power";
  climate["payload_on"] = "ON";
  climate["payload_off"] = "OFF";
  // HA's climate keys and our command/state field names are the same words.
  const char* climateFields[] = { "mode", "temperature", "fan_mode", "swing_mode", "swing_horizontal_mode" };
  for (const char* field : climateFields) {
    String prefix = field;
    climate[prefix + "_command_topic"] = commandPrefix + field;
    climate[prefix + "_state_topic"] = stateTopic;
    climate[prefix + "_state_template"] = String("{{ value_json['") + field + "'] }}";
  }

  for (const SwitchSpec& spec : kSwitches) {
    JsonObject c = addComponent(components, spec.key, "switch", spec.name);
    bindCommand(c, spec.key);
    bindState(c, spec.key);
    c["icon"] = spec.icon;
    if (!spec.enabledByDefault) c["enabled_by_default"] = false;
    if (spec.config) c["entity_category"] = "config";
  }

  JsonObject powerOnMode = addComponent(components, "power_on_mode", "select", "Power-On Mode");
  bindCommand(powerOnMode, "power_on_mode");
  bindState(powerOnMode, "power_on_mode");
  addNames(powerOnMode["options"].to<JsonArray>(), kPowerOnModes);
  powerOnMode["icon"] = "mdi:power";
  powerOnMode["entity_category"] = "config";

  JsonObject unit = addComponent(components, "unit", "select", "Temperature Unit");
  bindCommand(unit, "unit");
  bindState(unit, "unit");
  JsonArray units = unit["options"].to<JsonArray>();
  units.add("Celsius");
  units.add("Fahrenheit");
  unit["icon"] = "mdi:thermometer-lines";
  unit["entity_category"] = "config";

  JsonObject model = addComponent(components, "model", "number", "IR Model");
  bindCommand(model, "model");
  bindState(model, "model");
  model["min"] = -1;
  model["max"] = 100;
  model["mode"] = "box";
  model["icon"] = "mdi:numeric";
  model["entity_category"] = "config";

  JsonObject sendDelay = addComponent(components, "send_delay", "number", "Send Delay");
  bindCommand(sendDelay, "send_delay");
  bindState(sendDelay, "send_delay");
  sendDelay["min"] = 0;
  sendDelay["max"] = 5000;
  sendDelay["step"] = 100;
  sendDelay["mode"] = "slider";
  sendDelay["unit_of_measurement"] = "ms";
  sendDelay["icon"] = "mdi:timer-sand";
  sendDelay["entity_category"] = "config";

  JsonObject ignoreWindow = addComponent(components, "ignore_window", "number", "Echo Ignore Window");
  bindCommand(ignoreWindow, "ignore_window");
  bindState(ignoreWindow, "ignore_window");
  ignoreWindow["min"] = 100;
  ignoreWindow["max"] = 2000;
  ignoreWindow["step"] = 50;
  ignoreWindow["mode"] = "slider";
  ignoreWindow["unit_of_measurement"] = "ms";
  ignoreWindow["icon"] = "mdi:ear-hearing-off";
  ignoreWindow["entity_category"] = "config";
  ignoreWindow["enabled_by_default"] = false;

  JsonObject learn = addComponent(components, "learn", "button", "Learn From Remote");
  bindCommand(learn, "learn");
  learn["icon"] = "mdi:remote-tv";
  learn["entity_category"] = "config";

  JsonObject reconfigure = addComponent(components, "reconfigure", "button", "Open Setup Portal");
  bindCommand(reconfigure, "reconfigure");
  reconfigure["icon"] = "mdi:wifi-cog";
  reconfigure["entity_category"] = "config";

  JsonObject restart = addComponent(components, "restart", "button", "Restart");
  bindCommand(restart, "restart");
  restart["device_class"] = "restart";
  restart["entity_category"] = "config";

  JsonObject listening = addComponent(components, "learning", "binary_sensor", "Listening for Remote");
  listening["state_topic"] = stateTopic;
  listening["value_template"] = "{{ 'ON' if value_json.learning else 'OFF' }}";
  listening["icon"] = "mdi:access-point";
  listening["entity_category"] = "diagnostic";

  JsonObject remote = addComponent(components, "remote", "event", "Remote Button");
  remote["state_topic"] = eventTopic;
  JsonArray eventTypes = remote["event_types"].to<JsonArray>();
  eventTypes.add("remote_used");
  eventTypes.add("protocol_learned");
  remote["device_class"] = "button";
  remote["icon"] = "mdi:remote";

  JsonObject rssi = addComponent(components, "rssi", "sensor", "WiFi Signal");
  rssi["state_topic"] = diagnosticsTopic;
  rssi["value_template"] = "{{ value_json.rssi }}";
  rssi["device_class"] = "signal_strength";
  rssi["unit_of_measurement"] = "dBm";
  rssi["state_class"] = "measurement";
  rssi["entity_category"] = "diagnostic";

  JsonObject ip = addComponent(components, "ip", "sensor", "IP Address");
  ip["state_topic"] = diagnosticsTopic;
  ip["value_template"] = "{{ value_json.ip }}";
  ip["icon"] = "mdi:ip-network";
  ip["entity_category"] = "diagnostic";

  JsonObject uptime = addComponent(components, "uptime", "sensor", "Uptime");
  uptime["state_topic"] = diagnosticsTopic;
  uptime["value_template"] = "{{ value_json.uptime }}";
  uptime["device_class"] = "duration";
  uptime["unit_of_measurement"] = "s";
  uptime["state_class"] = "total_increasing";
  uptime["icon"] = "mdi:timer-outline";
  uptime["entity_category"] = "diagnostic";
  uptime["enabled_by_default"] = false;

  return publishJson("homeassistant/device/" + id + "/config", doc, true);
}

// The protocol list is ~80 entries, so it gets its own message to keep peak memory low on the ESP8266.
bool publishProtocolDiscovery() {
  JsonDocument doc;
  doc["unique_id"] = id + "_protocol";
  doc["name"] = "IR Protocol";
  doc["device"]["identifiers"].to<JsonArray>().add(id);
  doc["availability_topic"] = availabilityTopic;
  doc["command_topic"] = commandPrefix + "protocol";
  doc["state_topic"] = stateTopic;
  doc["value_template"] = "{{ value_json.protocol }}";
  doc["icon"] = "mdi:remote-tv";
  doc["entity_category"] = "config";

  JsonArray options = doc["options"].to<JsonArray>();
  options.add(kNoProtocol);
  for (int i = 0; i <= kLastDecodeType; i++) {
    decode_type_t protocol = static_cast<decode_type_t>(i);
    if (IRac::isProtocolSupported(protocol)) options.add(typeToString(protocol));
  }

  return publishJson("homeassistant/select/" + id + "_protocol/config", doc, true);
}

// Version 1 published one discovery topic per entity under "<location>_ac_*" and (by mistake) retained
// commands. Clear them once so Home Assistant doesn't keep a ghost device and the old commands don't replay.
void cleanupLegacyTopics() {
  String location = slugify(settings.room);
  if (location.isEmpty() || settings.legacyCleanedFor == location) return;

  const char* legacyEntities[][2] = {
    { "climate", "climate" },
    { "text", "protocol" },
    { "number", "model" },
    { "switch", "quiet" },
    { "switch", "turbo" },
    { "switch", "econo" },
    { "switch", "light" },
    { "switch", "filter" },
    { "switch", "clean" },
    { "switch", "beep" },
    { "number", "sleep" },
    { "number", "clock" },
    { "select", "command" },
    { "switch", "ifeel" },
    { "switch", "controller_echo" },
    { "number", "controller_ignore_window" },
    { "button", "controller_restart" },
    { "number", "controller_debounce_time" },
  };
  for (const auto& entity : legacyEntities) {
    String topic = String("homeassistant/") + entity[0] + "/" + location + "_ac_" + entity[1] + "/config";
    mqtt.publish(topic.c_str(), "", true);
  }

  const char* legacyCommands[] = { "power", "mode", "temperature", "fan_mode", "swing_mode", "swing_horizontal_mode" };
  for (const char* command : legacyCommands) {
    mqtt.publish((location + "/ac/set/" + command).c_str(), "", true);
  }
  mqtt.publish((location + "/ac/state").c_str(), "", true);
  mqtt.publish((location + "/ac/state/availability").c_str(), "", true);

  settings.legacyCleanedFor = location;
  settingsSave();
  Serial.printf("[mqtt] removed v1 topics for \"%s\"\n", location.c_str());
}

void publishDiagnostics() {
  JsonDocument doc;
  doc["rssi"] = WiFi.RSSI();
  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  publishJson(diagnosticsTopic, doc, true);
  lastDiagnosticsAt = millis();
}

void onMessage(char* topic, uint8_t* payload, size_t length) {
  char value[96];
  size_t copied = min(length, sizeof(value) - 1);
  memcpy(value, payload, copied);
  value[copied] = '\0';

  if (strcmp(topic, kStatusTopic) == 0) {
    if (strcmp(value, "online") == 0) {
      // Home Assistant restarted. Stagger slightly so a house full of devices doesn't answer at once.
      discoveryPending = true;
      discoveryAt = millis() + random(500, 3000);
    }
    return;
  }

  if (strncmp(topic, commandPrefix.c_str(), commandPrefix.length()) != 0) return;
  const char* field = topic + commandPrefix.length();
  if (copied == 0) return;  // a retained message being cleared

  Serial.printf("[mqtt] %s = %s\n", field, value);

  if (strcmp(field, "restart") == 0) {
    systemControl::requestRestart(false);
    return;
  }
  if (strcmp(field, "reconfigure") == 0) {
    systemControl::requestRestart(true);
    return;
  }

  switch (aircon::apply(field, value)) {
    case aircon::Result::kAppliedUnitChanged:
      discoveryPending = true;
      discoveryAt = millis();
      break;
    case aircon::Result::kInvalidValue:
      Serial.printf("[mqtt] invalid value for %s: %s\n", field, value);
      statePending = true;  // snap Home Assistant back to the real state
      break;
    case aircon::Result::kUnknownField:
      Serial.printf("[mqtt] unknown command topic: %s\n", topic);
      break;
    case aircon::Result::kApplied:
      break;
  }
}

bool resolveBroker() {
  if (!settings.mqttHost.isEmpty()) {
    brokerHost = settings.mqttHost;
    brokerPort = settings.mqttPort;
    mqtt.setServer(brokerHost.c_str(), brokerPort);
    return true;
  }

  if (brokerKnown && failedAttempts < 3) return true;

  Serial.println(F("[mqtt] looking for a broker on the network..."));
  if (!discoverBroker(brokerIp, brokerPort)) {
    Serial.println(F("[mqtt] no broker found via mDNS; set one in the setup portal"));
    brokerKnown = false;
    return false;
  }
  brokerKnown = true;
  failedAttempts = 0;
  brokerHost = brokerIp.toString();
  mqtt.setServer(brokerIp, brokerPort);
  Serial.printf("[mqtt] found broker at %s:%u\n", brokerHost.c_str(), brokerPort);
  return true;
}

void onConnected() {
  Serial.printf("[mqtt] connected to %s:%u\n", brokerHost.c_str(), brokerPort);
  failedAttempts = 0;
  retryDelayMs = 2000;

  mqtt.subscribe((commandPrefix + "#").c_str());
  mqtt.subscribe(kStatusTopic);
  mqtt.publish(availabilityTopic.c_str(), "online", true);

  cleanupLegacyTopics();
  publishDiscovery();
  statePending = true;
  publishDiagnostics();
}

void tryConnect() {
  if (!due(nextAttemptAt)) return;

  if (resolveBroker()) {
    const char* user = settings.mqttUser.isEmpty() ? nullptr : settings.mqttUser.c_str();
    const char* password = settings.mqttPassword.isEmpty() ? nullptr : settings.mqttPassword.c_str();
    if (mqtt.connect(id.c_str(), user, password, availabilityTopic.c_str(), 0, true, "offline")) {
      onConnected();
      return;
    }
    failedAttempts++;
    int state = mqtt.state();
    Serial.printf("[mqtt] connecting to %s:%u failed (%d%s)\n", brokerHost.c_str(), brokerPort, state,
                  state == MQTT_CONNECT_BAD_CREDENTIALS || state == MQTT_CONNECT_UNAUTHORIZED
                    ? ", check the MQTT username and password"
                    : "");
  }

  nextAttemptAt = millis() + retryDelayMs;
  retryDelayMs = min(retryDelayMs * 2, kMaxRetryDelayMs);
}

}  // namespace

void begin(const String& deviceId) {
  id = deviceId;
  String base = "aircon/" + id;
  stateTopic = base + "/state";
  availabilityTopic = base + "/availability";
  commandPrefix = base + "/set/";
  eventTopic = base + "/remote";
  diagnosticsTopic = base + "/diagnostics";

  mqtt.setBufferSize(1024);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(5);
  mqtt.setCallback(onMessage);

  aircon::onStateChanged(publishState);
  aircon::onRemoteEvent(publishRemoteEvent);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqtt.connected()) {
    tryConnect();
    return;
  }

  mqtt.loop();

  if (discoveryPending && due(discoveryAt)) {
    discoveryPending = false;
    publishDiscovery();
    statePending = true;
  }

  if (statePending) {
    JsonDocument doc;
    aircon::stateToJson(doc);
    if (publishJson(stateTopic, doc, true)) statePending = false;
  }

  if (millis() - lastDiagnosticsAt >= kDiagnosticsIntervalMs) publishDiagnostics();
}

bool connected() {
  return mqtt.connected();
}

String brokerAddress() {
  if (brokerHost.isEmpty()) return "";
  return brokerHost + ":" + brokerPort;
}

void publishState() {
  statePending = true;  // coalesced and sent from loop()
}

void publishRemoteEvent(const char* eventType) {
  if (!mqtt.connected()) return;
  JsonDocument doc;
  doc["event_type"] = eventType;
  publishJson(eventTopic, doc, false);
}

void publishDiscovery() {
  if (!mqtt.connected()) return;
  bool ok = publishDeviceDiscovery() && publishProtocolDiscovery();
  Serial.printf("[mqtt] discovery %s (free heap %u)\n", ok ? "published" : "failed, retrying in 15 s", ESP.getFreeHeap());
  if (!ok) {
    discoveryPending = true;
    discoveryAt = millis() + 15000;
  }
}

void goOffline() {
  if (!mqtt.connected()) return;
  mqtt.publish(availabilityTopic.c_str(), "offline", true);
  mqtt.disconnect();
}

}  // namespace homeAssistant
