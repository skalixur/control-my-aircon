# Control My Aircon

I want to control my aircon from anywhere. But how?

With the power of cheap Chinese electronics, of course.

This is an MQTT device which integrates with Home Assistant to control your aircon via IR through the IRremoteESP8266 library. A separate server is also in the repo to interface with MQTT over WebSockets, so you can control your aircon from anywhere with a web browser (without Home Assistant).

## Usage

1. Install dependencies.

- ArduinoJson 7.4.2 (Benoit Blanchon)
- IRremoteESP8266 2.8.6 (David Conran)
- PubSubClient3 (Nick O'Leary)
- ESP8266WiFi
- ESP8266WebServer
- ESP8266mDNS

2. Create `config.h`

Copy `config.example.h` to `config.h` and fill in your aircon's protocol, model, as well as your WiFi SSID and password.

3. Upload to your ESP8266.

4. Connect to the ESP8266's WiFi network.

5. Install Home Assistant and add the MQTT integration.

6. Home Assistant should auto-discover the device, and you can begin controlling your aircon.

_To use without Home Assistant, skip steps 5 and 6 and use the web interface instead._

## Components

- WiFi enabled microcontroller, in my case a NodeMCU (ESP8266)
- _n_ 940nm IR LEDs
- _n_ 60-220 ohm resistors (dependent on IR LED current ratings and output voltage, please calculate yourself!)
- 1x 10k ohm resistor
- 1x BJ(T) transistor for greater range (if necessary)
- 1x VS1838b IR receiver (though [TSOP](https://github.com/crankyoldgit/IRremoteESP8266/wiki/Frequently-Asked-Questions#Help_Im_getting_very_inconsistent_results_when_capturing_an_IR_message_using_a_VS1838b_IR_demodulator)s are preferred for reliability)

## Pinout

By default, the IR LED (or transistor gate) is connected to D1. The IR receiver is connected to D2. See the wiring diagram for more details.

These can be updated in the `config.h` file. See `config.example.h` for more details.

## Wiring Diagram

<img width="896" height="462" alt="image" src="https://github.com/user-attachments/assets/859c85bf-13ab-4b83-9320-cddcd3bd351e" />
