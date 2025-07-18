// === Serial Config ===
const uint32_t kBaudRate = 115200;

// === Pinout ===
const byte kIrLed = D1;         // IR LED output pin
const byte kIrReceiver = D2;    // IR receiver input pin

// === IR Redundancy Config ===
// Whether to echo received IR signals to keep state consistent
bool echo = true;
// Ignore IR input for X ms after sending to prevent feedback loop
unsigned long ignoreWindow = 300;

// === Aircon Protocol & Model ===
// See full list of protocols here: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRac.cpp
const decode_type_t protocol = decode_type_t::COOLIX;
// See full list of models here: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRsend.h
const int16_t model = 0;

// === WiFi Config ===
// Replace with your WiFi credentials
const char* ssid = "YOUR_SSID_HERE";
const char* password = "YOUR_PASSWORD_HERE";

// === Web Server Config ===
const uint16_t port = 80;                     // HTTP port
const char* mDnsName = "aircon";              // Accessible at http://aircon.local
String rootText = "This is the web server for my aircon.\r\n"
                  "Use endpoints GET /alive, GET /state, PUT /state, PATCH /state, POST /restart to control functionality.\r\n"
                  "Aircon Protocol: " + String(typeToString(protocol));

// === IR Receiver Config ===
const uint16_t kCaptureBufferSize = 1024;     // Raw buffer size
const uint8_t kTimeout = 50;                  // Timeout to consider end of transmission
const uint16_t kMinUnknownSize = 12;          // Ignore unknown messages shorter than this
const uint8_t kTolerancePercentage = kTolerance; // Defined by IRremoteESP8266; usually 25