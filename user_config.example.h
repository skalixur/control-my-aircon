// Optional compile-time overrides. Nothing here is required: by default the device opens a setup portal on
// first boot and learns your aircon from its remote.
//
// To use: copy this file to user_config.h (gitignored) and uncomment what you need.
// Full list of options and their defaults: src/defaults.h

// === Pins ===
// #define AIRCON_IR_LED_PIN D1
// #define AIRCON_IR_RECEIVER_PIN D2
// #define AIRCON_STATUS_LED_PIN -1      // -1 disables the status LED
// #define AIRCON_BUTTON_PIN -1          // -1 disables the FLASH-button shortcuts

// === Skip the setup portal ===
// Used only while the chip has no saved WiFi; to switch networks later, use the portal or a factory reset.
// #define AIRCON_WIFI_SSID "my-wifi"
// #define AIRCON_WIFI_PASSWORD "hunter2"
// #define AIRCON_DEFAULT_MQTT_USER "mqtt"
// #define AIRCON_DEFAULT_MQTT_PASSWORD "secret"
// #define AIRCON_DEFAULT_MQTT_HOST ""   // blank = find the broker automatically

// === Device ===
// #define AIRCON_DEFAULT_NAME "Bedroom Aircon"
// #define AIRCON_DEFAULT_ROOM "Bedroom"
// #define AIRCON_DEFAULT_PROTOCOL "COOLIX"   // skip learning from the remote
// #define AIRCON_DEFAULT_MODEL -1
// #define AIRCON_DEFAULT_FAHRENHEIT true
// #define AIRCON_DEFAULT_POWER_ON_MODE "Cool"   // "Last Used", "Cool", "Heat", "Auto", "Dry" or "Fan"

// === Security ===
// Protects OTA uploads (Arduino IDE network port, and http://<device>/update with username "admin").
// #define AIRCON_OTA_PASSWORD "change-me"
