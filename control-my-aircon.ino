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

#include "discovery.h"

bool sendDiscoveryPayloads() {
  bool p0 = publishDiscoveryPayloadClimate();
  bool p1 = publishDiscoveryPayloadText("Protocol", "protocol", "remote-tv");
  bool p2 = publishDiscoveryPayloadNumber("Model", "model", "identifier", -1, 100, "box");
  // Power
  // Mode
  // Degrees
  // Celsius
  // Fanspeed
  // Swing vertical
  // Swing horizontal
  bool p3 = publishDiscoveryPayloadSwitch("Quiet", "quiet", "volume-mute");
  bool p4 = publishDiscoveryPayloadSwitch("Turbo", "turbo", "car-turbocharger");
  bool p5 = publishDiscoveryPayloadSwitch("Econo", "econo", "sprout");
  bool p6 = publishDiscoveryPayloadSwitch("Light", "light", "lightbulb");
  bool p7 = publishDiscoveryPayloadSwitch("Filter", "filter", "air-filter");
  bool p8 = publishDiscoveryPayloadSwitch("Clean", "clean", "vacuum");
  bool p9 = publishDiscoveryPayloadSwitch("Beep", "beep", "home-sound-in");
  bool p10 = publishDiscoveryPayloadNumber("Sleep", "sleep", "bed-clock", -1, 32767, "box");
  bool p11 = publishDiscoveryPayloadNumber("Clock", "clock", "clock", -1, 1440, "box");
  bool p12 = publishDiscoveryPayloadSelectCommand("Command", "command", "tune");
  bool p13 = publishDiscoveryPayloadSwitch("iFeel", "iFeel", "home-thermometer");
  // Runtime config
  bool p14 = publishDiscoveryPayloadSwitch("Controller Echo", "echo", "repeat");
  bool p15 = publishDiscoveryPayloadNumber("Controller Ignore Window", "ignoreWindow", "timer-sand", 0, 2000, "slider");
  bool p16 = publishDiscoveryPayloadButton("Controller Restart", "restart", "restart", "restart");
  return p0 && p1 && p2 && p3 && p4 && p5 && p6 && p7 && p8 && p9 && p10 && p11 && p12 && p13 && p14 && p15 && p16;
}

String getAcStateJson(stdAc::state_t acState, bool pretty = false) {
  JsonDocument doc;

  doc["protocol"] = acState.protocol;
  doc["model"] = acState.model;
  doc["power"] = acState.power;
  doc["mode"] = ac.opmodeToString(acState.mode);
  doc["degrees"] = acState.degrees;
  doc["celsius"] = acState.celsius;
  doc["fanspeed"] = ac.fanspeedToString(acState.fanspeed);
  doc["swingv"] = ac.swingvToString(acState.swingv);
  doc["swingh"] = ac.swinghToString(acState.swingh);
  doc["quiet"] = acState.quiet;
  doc["turbo"] = acState.turbo;
  doc["econo"] = acState.econo;
  doc["light"] = acState.light;
  doc["filter"] = acState.filter;
  doc["clean"] = acState.clean;
  doc["beep"] = acState.beep;
  doc["sleep"] = acState.sleep;
  doc["clock"] = acState.clock;
  doc["command"] = ac.commandTypeToString(acState.command);
  doc["iFeel"] = acState.iFeel;
  doc["sensorTemperature"] = acState.sensorTemperature;

  String output;

  if (pretty) {
    serializeJsonPretty(doc, output);
  } else {
    serializeJson(doc, output);
  }

  return output;
}

bool sendState() {
  Serial.println("Attempting to send AC state to aircon:");
  Serial.println(getAcStateJson(ac.next, true));
  bool wasSuccessful = ac.sendAc();
  lastSentTime = millis();

  if (wasSuccessful) {
    Serial.println("Sent state to aircon.");
  } else {
    Serial.println("Failed to send state to aircon.");
  }
  return wasSuccessful;
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

bool publishFullState() {
  JsonDocument doc;

  stdAc::state_t currentState = ac.getStatePrev();

  // Climate
  doc["currentTemperature"] = currentState.sensorTemperature;
  doc["temperature"] = currentState.degrees;
  doc["fanMode"] = ac.fanspeedToString(currentState.fanspeed);
  doc["mode"] = ac.opmodeToString(currentState.mode);
  doc["swingHorizontalMode"] = ac.swinghToString(currentState.swingh);
  doc["swingMode"] = ac.swingvToString(currentState.swingv);

  doc["protocol"] = typeToString(currentState.protocol);
  doc["model"] = currentState.model;
  doc["quiet"] = currentState.quiet;
  doc["turbo"] = currentState.turbo;
  doc["econo"] = currentState.econo;
  doc["light"] = currentState.light;
  doc["filter"] = currentState.filter;
  doc["clean"] = currentState.clean;
  doc["beep"] = currentState.beep;
  doc["sleep"] = currentState.sleep;
  doc["clock"] = currentState.clock;
  doc["command"] = ac.commandTypeToString(currentState.command);
  doc["iFeel"] = currentState.iFeel;

  // Runtime config
  doc["echo"] = echo;
  doc["ignoreWindow"] = ignoreWindow;

  String output;

  serializeJson(doc, output);

  Serial.println("Publishing state: ");
  serializeJsonPretty(doc, Serial);
  Serial.println();

  return client.publish(stateTopic.c_str(), output.c_str(), true);
}

void onMqttConnect() {
  // Send the discovery payload so Home Assistant knows we exist
  bool successful = sendDiscoveryPayloads();
  client.subscribe((commandTopic + "/#").c_str());
  client.publish(availabilityTopic.c_str(), "online", true);
  publishFullState();
}

bool onOffToBool(String command) {
  return command == "ON" ? true : false;
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

  String cleanedTopic = String(topic).substring(commandTopic.length());
  Serial.println();
  Serial.println(cleanedTopic);

  if (cleanedTopic == "/beep") {
    ac.next.beep = onOffToBool(command);
  } else if (cleanedTopic == "/clean") {
    ac.next.clean = onOffToBool(command);
  } else if (cleanedTopic == "/controller_echo") {
    echo = onOffToBool(command);
  } else if (cleanedTopic == "/econo") {
    ac.next.econo = onOffToBool(command);
  } else if (cleanedTopic == "/filter") {
    ac.next.filter = onOffToBool(command);
  } else if (cleanedTopic == "/ifeel") {
    ac.next.iFeel = onOffToBool(command);
  } else if (cleanedTopic == "/light") {
    ac.next.light = onOffToBool(command);
  } else if (cleanedTopic == "/quiet") {
    ac.next.quiet = onOffToBool(command);
  } else if (cleanedTopic == "/turbo") {
    ac.next.turbo = onOffToBool(command);
  } else if (cleanedTopic == "/clock") {
    ac.next.clock = command.toInt();
  } else if (cleanedTopic == "/command") {
    ac.next.command = ac.strToCommandType(command.c_str());
  } else if (cleanedTopic == "/controller_ignore_window") {
    ignoreWindow = command.toInt();
  } else if (cleanedTopic == "/clock") {
    ac.next.clock = command.toFloat();
  } else if (cleanedTopic == "/command") {
    ac.next.command = ac.strToCommandType(command.c_str());
  } else if (cleanedTopic == "/model") {
    ac.next.model = ac.strToModel(command.c_str());
  } else if (cleanedTopic == "/protocol") {
    ac.next.protocol = strToDecodeType(command.c_str());
  } else if (cleanedTopic == "/sleep") {
    ac.next.sleep = command.toInt();
  } else if (cleanedTopic == "/temperature") {
    ac.next.degrees = command.toFloat();
  } else if (cleanedTopic == "/fan_mode") {
    ac.next.fanspeed = ac.strToFanspeed(command.c_str());
  } else if (cleanedTopic == "/mode") {
    ac.next.mode = ac.strToOpmode(command.c_str());

    if (ac.next.mode == stdAc::opmode_t::kOff) ac.next.power = false;
    else ac.next.power = true;

  } else if (cleanedTopic == "/swing_horizontal_mode") {
    ac.next.swingh = ac.strToSwingH(command.c_str());
  } else if (cleanedTopic == "/swing_mode") {
    ac.next.swingv = ac.strToSwingV(command.c_str());
  } else if (cleanedTopic == "/controller_restart") {
    restart();
  }

  Serial.println();
  sendState();
  publishFullState();
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
  ac.next.model = model;
  ac.next.celsius = celsius;

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
      IRAcUtils::decodeToState(&results, &ac.next);
      ac.next.protocol = savedProtocol;
      ac.next.model = savedModel;
      ac.markAsSent();
    }

    if(!ac.getState().power) {
      ac.next.mode = stdAc::opmode_t::kOff;
      ac.markAsSent();
    }

    Serial.println("Command received, new internal AC state: ");
    Serial.println(getAcStateJson(ac.getStatePrev(), true));
    publishFullState();
  }
}
