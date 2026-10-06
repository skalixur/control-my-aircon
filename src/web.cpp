#include "web.h"

#include <ArduinoJson.h>

#include "aircon.h"
#include "defaults.h"
#include "home_assistant.h"
#include "platform.h"
#include "settings.h"
#include "system.h"
#include "web_page.h"

namespace web {
namespace {

WebServerType server(80);
UpdateServerType updateServer;
String id;

void sendState() {
  JsonDocument doc;
  aircon::stateToJson(doc);

  JsonObject device = doc["device"].to<JsonObject>();
  device["id"] = id;
  device["name"] = settings.name;
  device["room"] = settings.room;
  device["version"] = AIRCON_VERSION;
  device["mqtt"] = homeAssistant::connected();
  device["broker"] = homeAssistant::brokerAddress();

  JsonObject options = doc["options"].to<JsonObject>();
  addNames(options["fan_mode"].to<JsonArray>(), kFanSpeeds);
  addNames(options["swing_mode"].to<JsonArray>(), kSwingV);
  addNames(options["swing_horizontal_mode"].to<JsonArray>(), kSwingH);
  options["min_temp"] = aircon::minTemp();
  options["max_temp"] = aircon::maxTemp();

  String body;
  serializeJson(doc, body);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", body);
}

void handleSet() {
  // A custom header can't be sent cross-origin without a CORS preflight (which we never answer), so a random web
  // page can't POST here on the user's behalf.
  if (!server.hasHeader("X-Aircon")) {
    server.send(403, "text/plain", "Missing X-Aircon header");
    return;
  }

  String field = server.arg("field");
  String value = server.arg("value");

  if (field == "restart" || field == "reconfigure") {
    systemControl::requestRestart(field == "reconfigure");
    sendState();
    return;
  }

  switch (aircon::apply(field.c_str(), value.c_str())) {
    case aircon::Result::kUnknownField:
      server.send(404, "text/plain", "Unknown field");
      return;
    case aircon::Result::kInvalidValue:
      server.send(400, "text/plain", "Invalid value");
      return;
    case aircon::Result::kAppliedUnitChanged:
      homeAssistant::publishDiscovery();
      break;
    case aircon::Result::kApplied:
      break;
  }
  sendState();
}

}  // namespace

void begin(const String& deviceId) {
  id = deviceId;

  const char* headers[] = { "X-Aircon" };
  server.collectHeaders(headers, sizeof(headers) / sizeof(headers[0]));

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", kWebPage); });
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/set", HTTP_POST, handleSet);
  server.onNotFound([] {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  if (strlen(AIRCON_OTA_PASSWORD) > 0) {
    updateServer.setup(&server, "/update", "admin", AIRCON_OTA_PASSWORD);
  } else {
    updateServer.setup(&server, "/update");
  }

  server.begin();
  MDNS.addService("http", "tcp", 80);
}

void loop() {
  server.handleClient();
}

}  // namespace web
