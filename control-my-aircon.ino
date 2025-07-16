#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <IRac.h>
#include <IRutils.h>
#include "config.h"

const uint16_t kIrLed = D1;

WiFiEventHandler gotIpEventHandler;
ESP8266WebServer server(port);
IRac ac(kIrLed);

void handleRoot() {
  server.send(200, "text/plain", "I am alive!\r\n");
  Serial.println("Received request at root");
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
  Serial.begin(115200);
  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  gotIpEventHandler = WiFi.onStationModeGotIP(&handleGotIp);

  ac.next.protocol = protocol;
  ac.next.model = 1;
  ac.next.mode = stdAc::opmode_t::kCool;
  ac.next.celsius = true;
  ac.next.degrees = 25;
  ac.next.fanspeed = stdAc::fanspeed_t::kMedium;
  ac.next.swingv = stdAc::swingv_t::kOff;
  ac.next.swingh = stdAc::swingh_t::kOff;
  ac.next.light = false;
  ac.next.beep = false;
  ac.next.econo = false;
  ac.next.filter = false;
  ac.next.turbo = false;
  ac.next.quiet = false;
  ac.next.sleep = -1;
  ac.next.clean = false;
  ac.next.clock = -1;
  ac.next.power = false;

  // Ensure WiFi is connected before anything else
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (MDNS.begin(mDnsName)) {
    Serial.println("MDNS responder started sucessfully");
  }

  server.on('/alive', HTTP_GET, []() {
    server.send(200, "text/plain", "I am alive!")
  });

  server.method()

  Serial.println();

  server.begin();
}

void loop() {

  if (!ac.isProtocolSupported(protocol)) {
    Serial.println("Selected protocol is not supported by library!");
    delay(10000);
    return;
  }

  server.handleClient();
  MDNS.update();

  // Serial.println("Protocol " + String(protocol) + " / " + typeToString(protocol) + " is supported.");

  // ac.next.power = true;
  // Serial.println("Turning the aircon on");
  // ac.sendAc();
  // delay(5000);

  // ac.next.power = false;
  // Serial.println("Turning the aircon off");
  // ac.sendAc();
  // delay(5000);

  // Serial.println("Repeating\n");
}