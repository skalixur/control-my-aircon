// The on-board LED: a heartbeat of what the device is doing, without needing a serial monitor.
#pragma once

#include <Arduino.h>

namespace statusLed {

enum class Pattern {
  kOff,         // all good
  kSetup,       // setup portal open: slow blink, 1 s on / 1 s off
  kConnecting,  // no WiFi or no MQTT: double blink every 2 s
  kLearning,    // waiting for the remote: fast blink
  kResetArmed,  // FLASH button held long enough to factory reset: solid
};

void begin();
void loop();
void setPattern(Pattern pattern);
void flash();  // short blip for IR sent or received

}  // namespace statusLed
