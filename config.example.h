// === Serial Config ===
const uint32_t kBaudRate = 115200;

// === Pinout ===
const byte kIrLed = D1;       // IR LED output pin
const byte kIrReceiver = D2;  // IR receiver input pin

// === IR Redundancy Config ===
// Whether to echo received IR signals to keep state consistent
bool echo = true;
// Ignore IR input for X ms after sending to prevent feedback loop
int ignoreWindow = 300;

// === Aircon Config ===
// See full list of protocols here: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRac.cpp
const decode_type_t protocol = decode_type_t::COOLIX;
// See full list of models here: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRsend.h
const int16_t model = 0;
const bool celsius = true; // False: fahrenheit
const int minTemp = 7;
const int maxTemp = 35;

// === WiFi Config ===
// Replace with your WiFi credentials
const char* ssid = "YOUR_SSID_HERE";
const char* wifiPassword = "YOUR_PASSWORD_HERE";

// === Interface Config ===
const IPAddress server(172, 16, 0, 2);  // MQTT server address
const uint16_t port = 1883;             // MQTT server port

// MQTT login details
const char* username = "mqttuser";
const char* password = "12345";

const char* displayName = "Bedroom Aircon";
const char* location = "bedroom";  // or "living_room", etc. - used to denote the device's unique ID, topic paths, and MDNS name

const String uniqueId = String(location) + "_ac";                               // e.g., "bedroom_ac" --> The name of the DEVICE
const String mDnsName = uniqueId;                                               // Accessible via http://bedroom_ac.local
const String commandTopic = String(location) + "/ac/set";                       // e.g., bedroom/ac/set — where HA sends commands
const String stateTopic = String(location) + "/ac/state";                       // e.g., bedroom/ac/state — where device publishes current state
const String availabilityTopic = stateTopic + "/availability";

// === IR Receiver Config ===
const uint16_t kCaptureBufferSize = 1024;         // Raw buffer size
const uint8_t kTimeout = 50;                      // Timeout to consider end of transmission
const uint16_t kMinUnknownSize = 12;              // Ignore unknown messages shorter than this
const uint8_t kTolerancePercentage = kTolerance;  // Defined by IRremoteESP8266; usually 25