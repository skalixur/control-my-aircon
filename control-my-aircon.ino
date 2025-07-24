#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRac.h>
#include <IRutils.h>
#include <PubSubClient.h>
#include "config.h"

unsigned long lastSentTime = 0;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiEventHandler gotIpEventHandler;
IRac ac(kIrLed);
IRrecv irrecv(kIrReceiver, kCaptureBufferSize, kTimeout, true);
decode_results results;
stdAc::state_t acState;

#include "discovery.h"

bool sendState() {
  Serial.println("Attempting to send AC state to aircon:");
  Serial.println(acStateToSerializedJsonString(ac .next, true));
  bool wasSuccessful = ac.sendAc();
  lastSentTime = millis();
  acState = ac.getState();
  if (wasSuccessful) {
    Serial.println("Sent state to aircon.");
  } else {
    Serial.println("Failed to send state to aircon.");
  }
  return wasSuccessful;
}

bool sendDiscoveryPayloads() {
  bool p0 = publishDiscoveryPayloadClimate();
  bool p1 = publishDiscoveryPayloadText("Protocol", "remote-tv");
  bool p2 = publishDiscoveryPayloadNumber("Model", "identifier", -1, 100, "box");
  // Power
  // Mode
  // Degrees
  // Celsius
  // Fanspeed
  // Swing vertical
  // Swing horizontal
  bool p3 = publishDiscoveryPayloadSwitch("Quiet", "volume-mute");
  bool p4 = publishDiscoveryPayloadSwitch("Turbo", "car-turbocharger");
  bool p5 = publishDiscoveryPayloadSwitch("Econo", "sprout");
  bool p6 = publishDiscoveryPayloadSwitch ("Light", "lightbulb");
  bool p7 = publishDiscoveryPayloadSwitch("Filter", "air-filter");
  bool p8 = publishDiscoveryPayloadSwitch("Clean", "vacuum");
  bool p9 = publishDiscoveryPayloadSwitch("Beep", "home-sound-in");
  bool p10 = publishDiscoveryPayloadNumber("Sleep", "bed-clock", -1, 32767, "box");
  bool p11 = publishDiscoveryPayloadNumber("Clock", "clock", -1, 1440, "box");
  bool p12 = publishDiscoveryPayloadText("Command", "tune");
  bool p13 = publishDiscoveryPayloadSwitch("iFeel", "home-thermometer");
  return p0 && p1 && p2 && p3 && p4 && p5 && p6 && p7 && p8 && p9 && p10 && p11 && p12 && p13;
}

void restart() {
  client.publish(availabilityTopic.c_str(), "offline", true);
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

void setupWifi() {
  // Network setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPassword);

  gotIpEventHandler = WiFi.onStationModeGotIP(&handleGotIp);

  delay(2000);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (MDNS.begin(mDnsName)) {
    Serial.println("MDNS responder started successfully.");
  } else {
    Serial.println("MDNS responder failed to start.");
  }

  Serial.println();
}

bool connectMqtt() {
  return client.connect(uniqueId.c_str(), username, password, availabilityTopic.c_str(), 0, true, "offline");
}

void onMqttConnect() {
  // Send the discovery payload so Home Assistant knows we exist
  bool successful = sendDiscoveryPayloads();
  client.publish(availabilityTopic.c_str(), "online", true);
  client.subscribe((commandTopic + "/#").c_str());
}

void callback(char* topic, uint8_t* payload, size_t plength) {

  String command = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (size_t i = 0; i < plength; i++) {
    command += (char)payload[i];
    Serial.print((char)payload[i]);
  }
  String newTopic = String(topic);
  newTopic.replace("set", "state");

  client.publish(newTopic.c_str(), command.c_str());

  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (connectMqtt()) {
      Serial.println("connected!");
      onMqttConnect();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(kBaudRate);

  // Device setup
  irrecv.setTolerance(kTolerancePercentage);
  irrecv.enableIRIn();

  client.setServer(server, port);
  client.setCallback(callback);

  ac.next.protocol = protocol;

  acState = ac.getState();

  // Ensure WiFi is connected before anything else
  setupWifi();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
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
}
