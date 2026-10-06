#include "aircon.h"

#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

#include "defaults.h"
#include "platform.h"
#include "settings.h"
#include "status_led.h"

namespace aircon {
namespace {

const uint16_t kCaptureBufferSize = 1024;
const uint8_t kCaptureTimeoutMs = 50;  // long enough to keep multi-frame AC messages together
const uint32_t kLearnTimeoutMs = 60000;
const uint32_t kPersistDelayMs = 10000;  // batch flash writes while someone is fiddling with the controls
const char* kStatePath = "/state.json";

IRac ac(AIRCON_IR_LED_PIN);
IRrecv irrecv(AIRCON_IR_RECEIVER_PIN, kCaptureBufferSize, kCaptureTimeoutMs, true);
decode_results results;

bool sendPending = false;
bool forceSend = false;
uint32_t lastChangeAt = 0;
uint32_t lastSentAt = 0;

bool learning = false;
uint32_t learningSince = 0;

bool persistPending = false;
uint32_t persistRequestedAt = 0;

void (*stateChangedCallback)() = nullptr;
void (*remoteEventCallback)(const char*) = nullptr;

bool isOn(const char* value) {
  return strcasecmp(value, "ON") == 0 || strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0;
}

float toUnit(float degrees, bool fromCelsius, bool toCelsius) {
  if (fromCelsius == toCelsius) return degrees;
  return toCelsius ? roundf((degrees - 32) * 5 / 9) : roundf(degrees * 9 / 5 + 32);
}

void announce() {
  if (stateChangedCallback) stateChangedCallback();
}

void stateChanged() {
  persistPending = true;
  persistRequestedAt = millis();
  announce();
}

void scheduleSend(bool force) {
  sendPending = true;
  forceSend = forceSend || force;
  lastChangeAt = millis();
}

void syncSettingsIntoState() {
  // Normalizes hand-typed names ("coolix" -> "COOLIX") and drops ones IRac can't send (e.g. "NEC"), which would
  // otherwise block learning and fail every send.
  decode_type_t protocol = strToDecodeType(settings.protocol.c_str());
  if (protocol != decode_type_t::UNKNOWN && !IRac::isProtocolSupported(protocol)) {
    Serial.printf("[ir] \"%s\" isn't an aircon protocol IRremoteESP8266 can send; ignoring it\n",
                  settings.protocol.c_str());
    protocol = decode_type_t::UNKNOWN;
  }
  settings.protocol = protocol == decode_type_t::UNKNOWN ? String() : typeToString(protocol);
  ac.next.protocol = protocol;
  ac.next.model = settings.model;
  ac.next.celsius = !settings.fahrenheit;
}

void saveState() {
  JsonDocument doc;
  const stdAc::state_t& s = ac.next;
  doc["power"] = s.power;
  doc["mode"] = static_cast<int>(s.mode);
  doc["degrees"] = s.degrees;
  doc["fan"] = static_cast<int>(s.fanspeed);
  doc["swingv"] = static_cast<int>(s.swingv);
  doc["swingh"] = static_cast<int>(s.swingh);
  doc["sleep"] = s.sleep;
  for (const Feature& f : kFeatures) doc[f.key] = s.*f.member;

  File file = LittleFS.open(kStatePath, "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
}

void loadState() {
  File file = LittleFS.open(kStatePath, "r");
  if (!file) return;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) return;

  stdAc::state_t& s = ac.next;
  s.power = doc["power"] | s.power;
  s.mode = static_cast<stdAc::opmode_t>(doc["mode"] | static_cast<int>(s.mode));
  s.degrees = doc["degrees"] | s.degrees;
  s.fanspeed = static_cast<stdAc::fanspeed_t>(doc["fan"] | static_cast<int>(s.fanspeed));
  s.swingv = static_cast<stdAc::swingv_t>(doc["swingv"] | static_cast<int>(s.swingv));
  s.swingh = static_cast<stdAc::swingh_t>(doc["swingh"] | static_cast<int>(s.swingh));
  s.sleep = doc["sleep"] | s.sleep;
  for (const Feature& f : kFeatures) s.*f.member = doc[f.key] | (s.*f.member);
}

void transmit(bool force) {
  if (!hasProtocol()) {
    Serial.println(F("[ir] no protocol yet; press any button on your aircon remote while pointing it at me"));
    return;
  }
  if (!force && !ac.hasStateChanged()) return;

  // Changing the temperature or fan while the unit is off shouldn't make it beep; the new settings go out with the
  // next power-on.
  if (!ac.next.power && !ac.getStatePrev().power) return;

  if (ac.sendAc()) {
    lastSentAt = millis();
    statusLed::flash();
    Serial.printf("[ir] sent %s: %s %.0f°, fan %s\n", typeToString(ac.next.protocol).c_str(),
                  ac.next.power ? nameOf(kModes, ac.next.mode) : "off", ac.next.degrees,
                  nameOf(kFanSpeeds, ac.next.fanspeed));
  } else {
    Serial.printf("[ir] %s can't be sent by IRremoteESP8266 (model %d)\n",
                  typeToString(ac.next.protocol).c_str(), ac.next.model);
  }
}

void receive() {
  if (!irrecv.decode(&results)) return;

  // Our own transmission bouncing back off the walls.
  if (millis() - lastSentAt < settings.ignoreWindowMs) return;

  stdAc::state_t decoded;
  if (!IRAcUtils::decodeToState(&results, &decoded, &ac.next)) return;  // not an aircon (TV remote, noise...)

  bool learned = false;
  if (learning || !hasProtocol()) {
    settings.protocol = typeToString(decoded.protocol);
    settings.model = decoded.model;
    settingsSave();
    learning = false;
    learned = true;
    Serial.printf("[ir] learned protocol %s, model %d\n", settings.protocol.c_str(), settings.model);
  } else if (decoded.protocol != ac.next.protocol) {
    Serial.printf("[ir] ignoring %s remote (expecting %s)\n", typeToString(decoded.protocol).c_str(),
                  typeToString(ac.next.protocol).c_str());
    return;
  }

  // Decoders often misreport the model, so the configured one wins.
  decoded.protocol = strToDecodeType(settings.protocol.c_str());
  decoded.model = settings.model;

  // Some protocols report "off" as a mode instead of a power flag; keep the real mode for the next power-on.
  if (decoded.mode == stdAc::opmode_t::kOff) {
    decoded.power = false;
    decoded.mode = ac.next.mode;
  }

  bool celsius = !settings.fahrenheit;
  decoded.degrees = toUnit(decoded.degrees, decoded.celsius, celsius);
  decoded.celsius = celsius;

  ac.next = decoded;
  sendPending = false;
  forceSend = false;
  if (settings.repeatRemote && !learned) {
    // Mark first so the repeat carries the absolute state only. Otherwise IRac turns every field the remote just
    // changed into a toggle command (COOLIX light/turbo, Whirlpool power...) and the repeat flips them back.
    ac.markAsSent();
    scheduleSend(true);
  } else {
    ac.markAsSent();
  }

  statusLed::flash();
  Serial.printf("[ir] remote: %s %.0f°\n", ac.next.power ? nameOf(kModes, ac.next.mode) : "off", ac.next.degrees);

  stateChanged();
  if (remoteEventCallback) remoteEventCallback(learned ? "protocol_learned" : "remote_used");
}

}  // namespace

void begin() {
  // IRsend only configures the pin on the first transmission; don't leave a transistor base floating until then.
  pinMode(AIRCON_IR_LED_PIN, OUTPUT);
  digitalWrite(AIRCON_IR_LED_PIN, LOW);

  irrecv.setUnknownThreshold(12);
  irrecv.setTolerance(kTolerance);
  irrecv.enableIRIn();

  loadState();
  syncSettingsIntoState();
  ac.markAsSent();  // the aircon already is in this state; don't beep at boot

  if (hasProtocol()) {
    Serial.printf("[ir] ready: %s, model %d\n", settings.protocol.c_str(), settings.model);
  } else {
    Serial.println(F("[ir] no protocol yet; press any button on your aircon remote while pointing it at me"));
  }
}

void loop() {
  uint32_t now = millis();

  if (learning && now - learningSince > kLearnTimeoutMs) {
    learning = false;
    Serial.println(F("[ir] learning timed out"));
    announce();
  }

  if (sendPending && now - lastChangeAt >= settings.sendDelayMs) {
    bool force = forceSend;
    sendPending = false;
    forceSend = false;
    transmit(force);
  }

  receive();

  if (persistPending && now - persistRequestedAt >= kPersistDelayMs) {
    persistPending = false;
    saveState();
  }
}

Result apply(const char* field, const char* value) {
  stdAc::state_t& s = ac.next;

  // === Aircon controls: these get transmitted ===
  if (strcmp(field, "power") == 0) {
    bool turningOn = isOn(value) && !s.power;
    s.power = isOn(value);
    if (turningOn) {
      stdAc::opmode_t resume = stdAc::opmode_t::kOff;
      valueOf(kPowerOnModes, settings.powerOnMode.c_str(), resume);
      if (resume != stdAc::opmode_t::kOff) s.mode = resume;
    }
    if (s.power && s.mode == stdAc::opmode_t::kOff) s.mode = stdAc::opmode_t::kCool;
  } else if (strcmp(field, "mode") == 0) {
    if (strcasecmp(value, "off") == 0) {
      s.power = false;
    } else {
      if (!valueOf(kModes, value, s.mode)) return Result::kInvalidValue;
      s.power = true;
    }
  } else if (strcmp(field, "temperature") == 0) {
    char* end;
    float degrees = strtof(value, &end);
    if (end == value) return Result::kInvalidValue;
    s.degrees = constrain(roundf(degrees), minTemp(), maxTemp());
  } else if (strcmp(field, "fan_mode") == 0) {
    if (!valueOf(kFanSpeeds, value, s.fanspeed)) return Result::kInvalidValue;
  } else if (strcmp(field, "swing_mode") == 0) {
    if (!valueOf(kSwingV, value, s.swingv)) return Result::kInvalidValue;
  } else if (strcmp(field, "swing_horizontal_mode") == 0) {
    if (!valueOf(kSwingH, value, s.swingh)) return Result::kInvalidValue;
  } else if (strcmp(field, "sleep") == 0) {
    s.sleep = isOn(value) ? 0 : -1;
  } else {
    bool isFeature = false;
    for (const Feature& f : kFeatures) {
      if (strcmp(field, f.key) == 0) {
        s.*f.member = isOn(value);
        isFeature = true;
        break;
      }
    }

    if (!isFeature) {
      // === Device configuration: saved, never transmitted ===
      Result result = Result::kApplied;

      if (strcmp(field, "protocol") == 0) {
        decode_type_t protocol =
            strcasecmp(value, kNoProtocol) == 0 ? decode_type_t::UNKNOWN : strToDecodeType(value);
        if (protocol == decode_type_t::UNKNOWN) {
          settings.protocol = "";
        } else if (IRac::isProtocolSupported(protocol)) {
          settings.protocol = typeToString(protocol);
        } else {
          return Result::kInvalidValue;
        }
      } else if (strcmp(field, "power_on_mode") == 0) {
        stdAc::opmode_t mode;
        if (!valueOf(kPowerOnModes, value, mode)) return Result::kInvalidValue;
        settings.powerOnMode = nameOf(kPowerOnModes, mode);
      } else if (strcmp(field, "model") == 0) {
        settings.model = atoi(value);
      } else if (strcmp(field, "unit") == 0) {
        bool fahrenheit = strcasecmp(value, "Fahrenheit") == 0;
        if (!fahrenheit && strcasecmp(value, "Celsius") != 0) return Result::kInvalidValue;
        s.degrees = toUnit(s.degrees, !settings.fahrenheit, !fahrenheit);
        settings.fahrenheit = fahrenheit;
        result = Result::kAppliedUnitChanged;
      } else if (strcmp(field, "repeat_remote") == 0) {
        settings.repeatRemote = isOn(value);
      } else if (strcmp(field, "send_delay") == 0) {
        settings.sendDelayMs = constrain(atoi(value), 0, 5000);
      } else if (strcmp(field, "ignore_window") == 0) {
        settings.ignoreWindowMs = constrain(atoi(value), 100, 2000);  // below ~100 ms we'd hear our own IR
      } else if (strcmp(field, "learn") == 0) {
        startLearning();
        return Result::kApplied;
      } else {
        return Result::kUnknownField;
      }

      settingsSave();
      syncSettingsIntoState();
      if (result == Result::kAppliedUnitChanged) s.degrees = constrain(s.degrees, minTemp(), maxTemp());
      stateChanged();
      return result;
    }
  }

  scheduleSend(false);
  stateChanged();
  return Result::kApplied;
}

void stateToJson(JsonDocument& doc) {
  const stdAc::state_t& s = ac.next;
  doc["mode"] = s.power ? nameOf(kModes, s.mode) : "off";
  doc["temperature"] = s.degrees;
  doc["fan_mode"] = nameOf(kFanSpeeds, s.fanspeed);
  doc["swing_mode"] = nameOf(kSwingV, s.swingv);
  doc["swing_horizontal_mode"] = nameOf(kSwingH, s.swingh);
  for (const Feature& f : kFeatures) doc[f.key] = (s.*f.member) ? "ON" : "OFF";
  doc["sleep"] = s.sleep >= 0 ? "ON" : "OFF";

  doc["protocol"] = hasProtocol() ? settings.protocol.c_str() : kNoProtocol;
  doc["power_on_mode"] = settings.powerOnMode;
  doc["model"] = settings.model;
  doc["unit"] = settings.fahrenheit ? "Fahrenheit" : "Celsius";
  doc["repeat_remote"] = settings.repeatRemote ? "ON" : "OFF";
  doc["send_delay"] = settings.sendDelayMs;
  doc["ignore_window"] = settings.ignoreWindowMs;
  doc["learning"] = learning || !hasProtocol();
}

void flush() {
  if (!persistPending) return;
  persistPending = false;
  saveState();
}

void startLearning() {
  learning = true;
  learningSince = millis();
  Serial.println(F("[ir] learning: press any button on your aircon remote within 60 s"));
  announce();
}

bool isLearning() {
  return learning || !hasProtocol();
}

bool hasProtocol() {
  return ac.next.protocol != decode_type_t::UNKNOWN;
}

float minTemp() {
  return settings.fahrenheit ? AIRCON_MIN_TEMP_F : AIRCON_MIN_TEMP_C;
}

float maxTemp() {
  return settings.fahrenheit ? AIRCON_MAX_TEMP_F : AIRCON_MAX_TEMP_C;
}

void onStateChanged(void (*callback)()) {
  stateChangedCallback = callback;
}

void onRemoteEvent(void (*callback)(const char*)) {
  remoteEventCallback = callback;
}

}  // namespace aircon
