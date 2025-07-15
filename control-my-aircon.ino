#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>
#include "config.h"

const uint16_t kIrLed = D1;
IRac ac(kIrLed);

void setup() {
  Serial.begin(115200);
  delay(200);

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
}

void loop() {

  if (!ac.isProtocolSupported(protocol)) {
    Serial.println("Selected protocol is not supported by library!");
    return;
  }

  Serial.println("Protocol " + String(protocol) + " / " + typeToString(protocol) + " is supported.");

  ac.next.power = true;
  Serial.println("Turning the aircon on");
  ac.sendAc();
  delay(1000);

  ac.next.power = false;
  Serial.println("Turning the aircon off");
  ac.sendAc();
  delay(1000);

  Serial.println("Repeating\n");
}