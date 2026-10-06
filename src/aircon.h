// The IR side: owns the IRac sender and IRrecv receiver, debounces outgoing changes, follows the physical
// remote, and converts state to and from the strings Home Assistant and the local page use.
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IRac.h>

template <typename T>
struct NamedValue {
  T value;
  const char* name;
};

// The order in each table is the order Home Assistant shows them in.
// Modes must be Home Assistant's own HVAC mode ids (it translates them, and voice assistants map them).
static const NamedValue<stdAc::opmode_t> kModes[] = {
  { stdAc::opmode_t::kCool, "cool" },
  { stdAc::opmode_t::kHeat, "heat" },
  { stdAc::opmode_t::kAuto, "auto" },
  { stdAc::opmode_t::kDry, "dry" },
  { stdAc::opmode_t::kFan, "fan_only" },
};

// Fan and swing options are shown verbatim in Home Assistant, the local page and voice assistants
// ("set the fan to Low"), so they are written for people.
static const NamedValue<stdAc::fanspeed_t> kFanSpeeds[] = {
  { stdAc::fanspeed_t::kAuto, "Auto" },
  { stdAc::fanspeed_t::kMin, "Min" },
  { stdAc::fanspeed_t::kLow, "Low" },
  { stdAc::fanspeed_t::kMedium, "Medium" },
  { stdAc::fanspeed_t::kMediumHigh, "Medium High" },
  { stdAc::fanspeed_t::kHigh, "High" },
  { stdAc::fanspeed_t::kMax, "Max" },
};

static const NamedValue<stdAc::swingv_t> kSwingV[] = {
  { stdAc::swingv_t::kOff, "Off" },
  { stdAc::swingv_t::kAuto, "Swing" },
  { stdAc::swingv_t::kHighest, "Highest" },
  { stdAc::swingv_t::kHigh, "High" },
  { stdAc::swingv_t::kUpperMiddle, "Upper Middle" },
  { stdAc::swingv_t::kMiddle, "Middle" },
  { stdAc::swingv_t::kLow, "Low" },
  { stdAc::swingv_t::kLowest, "Lowest" },
};

static const NamedValue<stdAc::swingh_t> kSwingH[] = {
  { stdAc::swingh_t::kOff, "Off" },
  { stdAc::swingh_t::kAuto, "Swing" },
  { stdAc::swingh_t::kLeftMax, "Far Left" },
  { stdAc::swingh_t::kLeft, "Left" },
  { stdAc::swingh_t::kMiddle, "Center" },
  { stdAc::swingh_t::kRight, "Right" },
  { stdAc::swingh_t::kRightMax, "Far Right" },
  { stdAc::swingh_t::kWide, "Wide" },
};

// What climate.turn_on (and "Hey Google, turn on the AC") resumes in. kOff means "whatever was last used".
static const NamedValue<stdAc::opmode_t> kPowerOnModes[] = {
  { stdAc::opmode_t::kOff, "Last Used" },
  { stdAc::opmode_t::kCool, "Cool" },
  { stdAc::opmode_t::kHeat, "Heat" },
  { stdAc::opmode_t::kAuto, "Auto" },
  { stdAc::opmode_t::kDry, "Dry" },
  { stdAc::opmode_t::kFan, "Fan" },
};

// Shown when no IR protocol has been set or learned yet. Also accepted as a command to clear it.
static const char* const kNoProtocol = "Not Set";

// Simple on/off features: JSON/topic key plus the state_t member it maps to.
struct Feature {
  const char* key;
  bool stdAc::state_t::*member;
};

static const Feature kFeatures[] = {
  { "quiet", &stdAc::state_t::quiet },
  { "turbo", &stdAc::state_t::turbo },
  { "econo", &stdAc::state_t::econo },
  { "light", &stdAc::state_t::light },
  { "filter", &stdAc::state_t::filter },
  { "clean", &stdAc::state_t::clean },
  { "beep", &stdAc::state_t::beep },
};

template <typename T, size_t N>
const char* nameOf(const NamedValue<T> (&table)[N], T value) {
  for (const auto& entry : table) {
    if (entry.value == value) return entry.name;
  }
  return table[0].name;
}

template <typename T, size_t N>
bool valueOf(const NamedValue<T> (&table)[N], const char* name, T& out) {
  for (const auto& entry : table) {
    if (strcasecmp(entry.name, name) == 0) {
      out = entry.value;
      return true;
    }
  }
  return false;
}

template <typename T, size_t N>
void addNames(JsonArray array, const NamedValue<T> (&table)[N]) {
  for (const auto& entry : table) array.add(entry.name);
}

namespace aircon {

enum class Result {
  kUnknownField,
  kInvalidValue,
  kApplied,
  kAppliedUnitChanged,  // Home Assistant needs fresh discovery (temperature unit and range)
};

void begin();
void loop();

// Applies one change from Home Assistant or the local page, e.g. ("mode", "cool") or ("quiet", "ON").
Result apply(const char* field, const char* value);

// Full state, as published to Home Assistant and served to the local page.
void stateToJson(JsonDocument& doc);

// Writes a pending state change to flash now (before a restart).
void flush();

void startLearning();
bool isLearning();
bool hasProtocol();
float minTemp();
float maxTemp();

// Called whenever the state changes (from any source).
void onStateChanged(void (*callback)());
// Called when the physical remote was used. eventType is "remote_used" or "protocol_learned".
void onRemoteEvent(void (*callback)(const char* eventType));

}  // namespace aircon
