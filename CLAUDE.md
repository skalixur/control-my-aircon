# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Arduino firmware for ESP8266 (NodeMCU) and ESP32 that controls an aircon over IR (IRremoteESP8266's `IRac`) and integrates with Home Assistant through MQTT discovery. The scope is Home Assistant first. The only web UI is a small self-contained fallback page served by the device. Don't add hosted or external web frontends.

## Build

There's no test suite or linter. Libraries: ArduinoJson 7, IRremoteESP8266 2.8.6, PubSubClient3 3.x, WiFiManager 2.0.17. Compile with the Arduino IDE's bundled CLI:

```sh
CLI="/c/Users/juanm/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
"$CLI" --config-file ~/.arduinoIDE/arduino-cli.yaml compile -b esp8266:esp8266:nodemcuv2 --output-dir "$TMP/aircon-esp8266" .
"$CLI" --config-file ~/.arduinoIDE/arduino-cli.yaml compile -b esp32:esp32:esp32:PartitionScheme=min_spiffs --output-dir "$TMP/aircon-esp32" .
```

ESP8266 IRAM sits at about 94%, so watch for growth. ESP32 needs a partition scheme with app slots of at least 1.3 MB.

## Layout and conventions

- The `.ino` file handles setup and loop, the WiFiManager portal, OTA, and the FLASH button. Everything else lives in `src/` (Arduino compiles it recursively), one namespace per module.
- `src/defaults.h` holds every compile-time option as an `#ifndef` default. It also includes an optional, gitignored `user_config.h` from the repo root. Add new options there and document them in `user_config.example.h`. The old gitignored `config.h` is v1's and is unused.
- Runtime settings live in `Settings` (`src/settings.*`) and persist to LittleFS `/settings.json`. The last AC state is saved separately to `/state.json`, with writes batched 10 s after a change to limit flash wear.
- `src/platform.h` holds all `#if ESP8266 / ESP32` differences.

## Data flow

- **Single command path:** `aircon::apply(field, value)` takes every change, whether from MQTT (`aircon/<id>/set/<field>`) or the local page (`POST /api/set`). For AC fields it updates `ac.next` and schedules a debounced send of `settings.sendDelayMs`. For config fields (protocol, model, unit, repeat_remote, send_delay, ignore_window, learn) it saves settings instead. A unit change returns `kAppliedUnitChanged` so discovery is republished.
- **State publishing:** `aircon::stateToJson` builds the single state document. It's retained on `aircon/<id>/state` and also served at `/api/state`. Any state change calls `onStateChanged`, which sets a flag in `homeAssistant`; the publish happens in `loop()`, so changes are coalesced.
- **Physical remote:** IR received from the remote is decoded with `IRAcUtils::decodeToState`. Remotes using a different protocol are ignored. Received IR within `ignoreWindowMs` of our own transmission is ignored. When learning, or when no protocol is set, the received protocol and model are adopted. The decoded state then becomes `ac.next`. It's either marked as sent or, if `repeatRemote` is on, re-sent.
- **String tables:** `kModes`, `kFanSpeeds`, `kSwingV`, `kSwingH` and `kFeatures` in `src/aircon.h` are the only source of the strings shared by state JSON, discovery option lists and the local page. Use these tables rather than IRac's `*ToString` helpers, which are locale-dependent and don't match Home Assistant.

## Home Assistant discovery

- **Messages:** one device discovery message goes to `homeassistant/device/<id>/config`. The protocol select goes in its own message, because its option list of about 80 entries is large. Messages are serialized into one malloc'd buffer and sent with `beginPublish`/`write`/`endPublish`, because PubSubClient's own buffer is only 1024 bytes and its per-byte `write` is slow.
- **When it's republished:** on every MQTT connect and on `homeassistant/status` = `online`.
- **Commands:** they are never retained. Empty payloads on command topics are ignored, because those are retained messages being cleared.
- **v1 cleanup:** `cleanupLegacyTopics()` deletes v1's `homeassistant/<component>/<room>_ac_*/config` and the retained `<room>/ac/...` topics. It runs once per room slug.

## Device identity

`deviceId` is `aircon_<last 3 MAC bytes>` and the hostname is `aircon-<same>`, so nothing per-device is compiled in.
