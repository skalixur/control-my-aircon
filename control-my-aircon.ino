#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRac.h>
#include <IRutils.h>
#include "config.h"

unsigned long lastSentTime = 0;

WiFiEventHandler gotIpEventHandler;
ESP8266WebServer server(port);
IRac ac(kIrLed);
IRrecv irrecv(kIrReceiver, kCaptureBufferSize, kTimeout, true);
decode_results results;
stdAc::state_t acState;

#include "ac_json.h"

void handleRoot() {
  decode_type_t currentAcProtocol = ac.getState().protocol;

  String newRootText;
  newRootText = rootText;
  newRootText += "\r\nAircon Protocol : ";
  newRootText += String(typeToString(currentAcProtocol));
  newRootText += "\r\nProtocol Supported? : ";
  newRootText += (ac.isProtocolSupported(currentAcProtocol) ? "true" : "false");

  sendWithCors(200, "text/plain", newRootText);
}

void handleGetAlive() {
  sendWithCors(200, "text/plain", "I am alive!");
}

void handleGetState() {
  String state = acStateToSerializedJsonString(acState);
  sendWithCors(200, "application/json", state);
}

void handlePutState() {
  String postBody = server.arg("plain");
  bool wasSuccessful = sendSerializedJsonAcState(postBody);
  if (wasSuccessful) {
    sendWithCors(200, "application/json", "{ \"ok\": true }");
  } else {
    sendWithCors(400, "application/json", "{ \"ok\": false, \"error\": \"Failed to send state to aircon. (Unsupported protocol?)\" }");
  }
}

void handleGetConfig() {
  String config = configToJsonString();
  sendWithCors(200, "application/json", config);
}

void handlePutConfig() {
  String postBody = server.arg("plain");
  bool error = setSerializedJsonConfig(postBody);

  if (error) {
    sendWithCors(400, "application/json", "{ \"ok\": false, \"error\": \"Failed to set config.\" }");
  } else {
    sendWithCors(200, "application/json", "{ \"ok\": true }");
  }
}

void handleSendState() {
  bool wasSuccessful = sendState();
  if (wasSuccessful) {
    sendWithCors(200, "application/json", "{ \"ok\": true }");
  } else {
    sendWithCors(400, "application/json", "{ \"ok\": false, \"error\": \"Failed to send state to aircon. (Unsupported protocol?)\" }");
  }
}

void restart() {
  sendWithCors(200, "text/plain", "Restarting...");
  delay(100);
  ESP.restart();
}

void handleGotIp(const WiFiEventStationModeGotIP& event) {
  Serial.print("Successfully connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: ");
  Serial.println(WiFi.hostname());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void setup() {
  Serial.begin(kBaudRate);

  // Device setup
  irrecv.setTolerance(kTolerancePercentage);
  irrecv.enableIRIn();

  ac.next.protocol = protocol;

  acState = ac.getState();

  // Network setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  gotIpEventHandler = WiFi.onStationModeGotIP(&handleGotIp);

  // Ensure WiFi is connected before anything else
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (MDNS.begin(mDnsName)) {
    Serial.println("MDNS responder started successfully.");
  } else {
    Serial.println("MDNS responser failed to start.")
  }

  Serial.println();

  // Server setup
  server.on("/", HTTP_GET, handleRoot);
  server.on("/alive", HTTP_GET, handleGetAlive);
  server.on("/state", HTTP_GET, handleGetState);
  server.on("/state", HTTP_PUT, handlePutState);
  server.on("/config", HTTP_GET, handleGetConfig);
  server.on("/config", HTTP_PUT, handlePutConfig);
  server.on("/resend", HTTP_POST, handleSendState);
  server.on("/restart", HTTP_POST, restart);

  server.begin();
}

void loop() {

  server.handleClient();
  MDNS.update();

  if (irrecv.decode(&results) && millis() - lastSentTime >= ignoreWindow) {
    decode_type_t savedProtocol = ac.getState().protocol;
    int16_t savedModel = ac.getState().model;
    if (echo) {
      IRAcUtils::decodeToState(&results, &(ac.next));
      // Don't affect the model as it frequently decodes the model/protocol incorrectly.
      // This, in essence, only allows it to be changed through config.h or a PUT request.
      ac.next.protocol = savedProtocol;
      ac.next.model = savedModel;
      sendState();
    } else {
      IRAcUtils::decodeToState(&results, &acState);
      acState.protocol = savedProtocol;
      acState.model = savedModel;
    }

    Serial.println("Command received, new internal AC state: ");
    Serial.println(acStateToSerializedJsonString(acState, true));
  }