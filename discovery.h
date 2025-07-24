
String clean(String label) {
  String cleanLabel = label;
  cleanLabel.trim();
  cleanLabel.toLowerCase();
  cleanLabel.replace(" ", "_");
  return cleanLabel;
}

void attachDeviceInfo(JsonDocument& doc) {
  JsonObject device = doc["device"].to<JsonObject>();
  device["identifiers"] = uniqueId;
  device["model"] = "IR AC Controller";
  device["name"] = displayName;
}

void attachAvailabilityInfo(JsonDocument& doc) {
  doc["availability_topic"] = availabilityTopic;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
}

String attachNameDependentInfo(JsonDocument& doc, String component, String label) {
  String cleanLabel = clean(label);

  String objectId;
  objectId = uniqueId + "_" + cleanLabel;

  doc["name"] = label;
  doc["unique_id"] = objectId;
  doc["object_id"] = objectId;

  String discoveryTopic;
  discoveryTopic = "homeassistant/" + component + "/" + objectId + "/config";  // e.g., homeassistant/climate/bedroom_ac_climate/config

  return discoveryTopic;
}

bool publishPayload(JsonDocument& doc, String topic) {
  String discoveryPayload;

  serializeJson(doc, discoveryPayload);
  serializeJsonPretty(doc, Serial);
  Serial.println();

  Serial.print("Sending discovery payload to:");
  Serial.print(topic);
  Serial.println("...");
  return client.publish(topic.c_str(), discoveryPayload.c_str(), true);
}

bool publishDiscoveryPayloadClimate() {
  JsonDocument doc;

  String discoveryTopic = attachNameDependentInfo(doc, "climate", "Climate");
  attachAvailabilityInfo(doc);
  attachDeviceInfo(doc);

  JsonArray modes = doc["modes"].to<JsonArray>();
  modes.add("off");
  modes.add("auto");
  modes.add("cool");
  modes.add("heat");
  modes.add("dry");
  modes.add("fan_only");

  JsonArray fanModes = doc["fan_modes"].to<JsonArray>();
  fanModes.add("Auto");
  fanModes.add("Minimum");
  fanModes.add("Low");
  fanModes.add("Medium");
  fanModes.add("Medium-High");
  fanModes.add("High");
  fanModes.add("Max");

  JsonArray swingHorizontalModes = doc["swing_horizontal_modes"].to<JsonArray>();
  swingHorizontalModes.add("Off");
  swingHorizontalModes.add("Auto");
  swingHorizontalModes.add("Left Max");
  swingHorizontalModes.add("Left");
  swingHorizontalModes.add("Middle");
  swingHorizontalModes.add("Right");
  swingHorizontalModes.add("Right Max");
  swingHorizontalModes.add("Wide");

  JsonArray swingModes = doc["swing_modes"].to<JsonArray>();
  swingModes.add("Off");
  swingModes.add("Auto");
  swingModes.add("Highest");
  swingModes.add("High");
  swingModes.add("Upper Middle");
  swingModes.add("Middle");
  swingModes.add("Low");
  swingModes.add("Lowest");

  doc["optimistic"] = true;  // debug
  doc["retain"] = true;

  doc["payload_on"] = "ON";  // shouldn't be necessary
  doc["payload_off"] = "RESTART";

  doc["temperature_unit"] = "C";
  if (!ac.getState().celsius) {
    doc["temperature_unit"] = "F";
  }

  doc["power_command_topic"] = commandTopic + "/power";

  doc["current_temperature_topic"] = stateTopic;
  doc["current_temperature_template"] = "{{ value_json.currentTemperature }}"; // currentTemperature

  doc["temperature_state_topic"] = stateTopic;
  doc["temperature_state_template"] = "{{value_json.temperature}}"; // temperature

  doc["fan_mode_state_topic"] = stateTopic;
  doc["fan_mode_state_template"] = "{{value_json.fanMode}}"; // fanMode

  doc["mode_state_topic"] = stateTopic;
  doc["mode_state_template"] = "{{value_json.mode}}"; // mode

  doc["swing_horizontal_mode_state_topic"] = stateTopic;
  doc["swing_horizontal_mode_state_template"] = "{{ value_json.swingHorizontalMode }}"; // swingHorizontalMode

  doc["swing_mode_state_topic"] = stateTopic;
  doc["swing_mode_state_template"] = "{{ value_json.swingMode }}"; // swingMode

  //TODO: INSERT TEMPLATES

  doc["temperature_command_topic"] = commandTopic + "/temperature";
  doc["fan_mode_command_topic"] = commandTopic + "/fan_mode";
  doc["mode_command_topic"] = commandTopic + "/mode";
  doc["swing_horizontal_mode_command_topic"] = commandTopic + "/swing_horizontal_mode";
  doc["swing_mode_command_topic"] = commandTopic + "/swing_mode";

  return publishPayload(doc, discoveryTopic);
}

bool publishDiscoveryPayloadSwitch(String label, String templateName, String icon) {
  JsonDocument doc;
  String component = "switch";

  String discoveryTopic = attachNameDependentInfo(doc, component, label);
  attachAvailabilityInfo(doc);
  attachDeviceInfo(doc);

  String cleanLabel = clean(label);

  doc["command_topic"] = commandTopic + "/" + cleanLabel;
  doc["state_topic"] = stateTopic;
  doc["value_template"] = "{{ value_json." + templateName + " }}";
  doc["platform"] = component;
  doc["payload_off"] = "OFF";
  doc["payload_on"] = "ON";
  doc["icon"] = "mdi:" + icon;

  return publishPayload(doc, discoveryTopic);
}

bool publishDiscoveryPayloadNumber(String label, String templateName, String icon, int min, int max, String mode) {
  JsonDocument doc;
  String component = "number";

  String discoveryTopic = attachNameDependentInfo(doc, component, label);
  attachAvailabilityInfo(doc);
  attachDeviceInfo(doc);

  String cleanLabel = clean(label);

  doc["command_topic"] = commandTopic + "/" + cleanLabel;
  doc["state_topic"] = stateTopic;
  doc["value_template"] = "{{ value_json." + templateName + " }}";
  //TODO: insert template
  doc["platform"] = component;
  doc["payload_off"] = "OFF";
  doc["payload_on"] = "ON";
  doc["icon"] = "mdi:" + icon;

  doc["min"] = min;
  doc["max"] = max;
  doc["step"] = 1;
  doc["mode"] = mode;

  return publishPayload(doc, discoveryTopic);
}

bool publishDiscoveryPayloadText(String label, String templateName, String icon) {
  JsonDocument doc;
  String component = "text";

  String discoveryTopic = attachNameDependentInfo(doc, component, label);
  attachAvailabilityInfo(doc);
  attachDeviceInfo(doc);

  String cleanLabel = clean(label);

  doc["command_topic"] = commandTopic + "/" + cleanLabel;
  doc["state_topic"] = stateTopic;
  doc["value_template"] = "{{ value_json." + templateName + " }}";
  //TODO: insert template
  doc["platform"] = component;
  doc["payload_off"] = "OFF";
  doc["paylod_on"] = "ON";
  doc["icon"] = "mdi:" + icon;

  return publishPayload(doc, discoveryTopic);
}