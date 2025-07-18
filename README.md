# Control My Aircon 

I want to control my aircon from anywhere. But how?

With the power of cheap Chinese electronics, of course.

## Usage

1. Install dependencies.

- ArduinoJson 7.4.2 (Benoit Blanchon)
- IRremoteESP8266 2.8.6 (David Conran)
- ESP8266WiFi
- ESP8266WebServer
- ESP8266mDNS

## Components

- WiFi enabled microcontroller, in my case a NodeMCU (ESP8266)
- *n* 940nm IR LEDs
- *n* 60-220 ohm resistors (dependent on IR LED current ratings and output voltage, please calculate yourself!)
- 1x 10k ohm resistor
- 1x BJ(T) transistor for greater range (if necessary)

## Pinout


