#include "system.h"

#include <Arduino.h>

#include "aircon.h"
#include "home_assistant.h"
#include "settings.h"

namespace systemControl {
namespace {

bool restartRequested = false;
uint32_t restartAt = 0;

}  // namespace

void requestRestart(bool openPortal) {
  if (openPortal) {
    settings.openPortalOnBoot = true;
    settingsSave();
  }
  Serial.printf("[system] restarting%s\n", openPortal ? " into the setup portal" : "");
  restartRequested = true;
  restartAt = millis() + 750;  // let the HTTP response / MQTT ack go out first
}

void loop() {
  if (!restartRequested || static_cast<int32_t>(millis() - restartAt) < 0) return;
  aircon::flush();
  homeAssistant::goOffline();
  delay(100);
  ESP.restart();
}

}  // namespace systemControl
