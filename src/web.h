// Local web server: the fallback control page, a tiny JSON API, and firmware upload at /update.
#pragma once

#include <Arduino.h>

namespace web {

void begin(const String& deviceId);
void loop();

}  // namespace web
