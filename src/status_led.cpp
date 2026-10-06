#include "status_led.h"

#include "defaults.h"

namespace statusLed {
namespace {

Pattern current = Pattern::kOff;
uint32_t flashUntil = 0;

void write(bool on) {
  if (AIRCON_STATUS_LED_PIN < 0) return;
  digitalWrite(AIRCON_STATUS_LED_PIN, on != AIRCON_STATUS_LED_INVERTED ? HIGH : LOW);
}

bool patternIsOn(uint32_t now) {
  switch (current) {
    case Pattern::kSetup:
      return now % 2000 < 1000;
    case Pattern::kConnecting: {
      uint32_t phase = now % 2000;
      return phase < 80 || (phase >= 240 && phase < 320);
    }
    case Pattern::kLearning:
      return now % 250 < 125;
    case Pattern::kResetArmed:
      return true;
    case Pattern::kOff:
    default:
      return false;
  }
}

}  // namespace

void begin() {
  if (AIRCON_STATUS_LED_PIN < 0) return;
  pinMode(AIRCON_STATUS_LED_PIN, OUTPUT);
  write(false);
}

void loop() {
  uint32_t now = millis();
  bool flashing = static_cast<int32_t>(flashUntil - now) > 0;
  write(flashing ? !patternIsOn(now) : patternIsOn(now));
}

void setPattern(Pattern pattern) {
  current = pattern;
}

void flash() {
  flashUntil = millis() + 60;
}

}  // namespace statusLed
