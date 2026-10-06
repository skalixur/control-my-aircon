// Restarts requested from Home Assistant or the local page, deferred so the request can be acknowledged first.
#pragma once

namespace systemControl {

// openPortal: come back up in setup-portal mode (WiFi and MQTT settings pre-filled).
void requestRestart(bool openPortal);
void loop();

}  // namespace systemControl
