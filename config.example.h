// Aircon protocol.
// See possible values here: https://github.com/crankyoldgit/IRremoteESP8266/blob/42eead875d707b0f03f6d7f897b4dbb3b7607486/src/IRac.cpp
const decode_type_t protocol = decode_type_t::COOLIX;
// WiFi SSID.
// Case sensitive. Must be exact.
const char* ssid = "My WiFi SSID";
// WiFi Password.
// Leave empty for no password.
const char* password = "asd123";
// Server Port.
const uint16_t port = 80;
const char* mDnsName = "aircon"; // -> aircon.local
// Root text.
// Text to be served to clients at the root path /
const char* rootText = "This is the web server for my aircon.\r\n";