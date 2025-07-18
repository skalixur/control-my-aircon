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

2. Create `config.h`

Copy `config.example.h` to `config.h` and fill in your aircon's protocol, model, as well as your WiFi SSID and password.

3. Upload to your ESP8266.

4. Connect to the ESP8266's WiFi network.

5. Send requests as needed to the server. See endpoints below for more details.

## Components

- WiFi enabled microcontroller, in my case a NodeMCU (ESP8266)
- _n_ 940nm IR LEDs
- _n_ 60-220 ohm resistors (dependent on IR LED current ratings and output voltage, please calculate yourself!)
- 1x 10k ohm resistor
- 1x BJ(T) transistor for greater range (if necessary)
- 1x VS1838b IR receiver (though [TSOP](https://github.com/crankyoldgit/IRremoteESP8266/wiki/Frequently-Asked-Questions#Help_Im_getting_very_inconsistent_results_when_capturing_an_IR_message_using_a_VS1838b_IR_demodulator)s are preferred for reliability)

## Pinout

By default, the IR LED (or transistor gate) is connected to D1. The IR receiver is connected to D2.

These can be updated in the `config.h` file. See `config.example.h` for more details.

## Endpoints

|  HTTP Method   |   Endpoint      |
|--------------- | --------------- |
|       GET      |       `/`       |
|       GET      |    `/alive`     |
|       GET      |    `/state`     |
|       PUT      |     `/state`    |
|       GET      |    `/config`    |
|       PUT      |    `/config`    |
|      POST      |    `/resend`    |
|      POST      |   `/restart`    |

### GET `/`

Returns `rootMessage` as defined in `config.h` as well as the current AC protocol and whether or not it is supported. Always responds with HTTP Code 200.

### GET `/alive`

Always responds with the text `I am alive!` and HTTP Code 200.

### GET `/state`

Returns the current state of the aircon, as a JSON object. Always responds with HTTP Code 200.

Example response:

```json
{
    "protocol": 49,
    "model": 0,
    "power": false,
    "mode": "Auto",
    "degrees": 26,
    "celsius": true,
    "fanspeed": "Auto",
    "swingv": "Auto",
    "swingh": "Auto",
    "quiet": false,
    "turbo": false,
    "econo": false,
    "light": false,
    "filter": false,
    "clean": false,
    "beep": false,
    "sleep": -1,
    "clock": -1,
    "command": "Control",
    "iFeel": false,
    "sensorTemperature": -100
}
```

### PUT `/state`

Sets the internal state and external state of the aircon by sending it as an IR message. Responds with HTTP Code 200 unless the message failed to send.

The body of the request should be a JSON object. Not all properties are required to be present. Example body:

```json
{
    "power": true,
    "mode": "Cool",
    "degrees": 19
}
```

Example response:

```json
{
    "ok": true
}
```

### GET `/config`

Returns the current runtime-editable config, as a JSON object. Always responds with HTTP Code 200.

Example response:

```json
{
    "echo": true,
    "ignoreWindow": 300
}
```

### PUT `/config`

Sets the runtime-editable config of the server. Responds with HTTP Code 200 unless malformed JSON is received.

The body of the request should be a JSON object. Not all properties are required to be present. Example body:

```json
{
    "echo": false,
    "ignoreWindow": 10000
}
```

Example response:

```json
{
    "ok": true
}
```

### POST `/resend`

Resends the internal state of the aircon through an IR message. Calling this is not necessary in most scenarios as an IR message is already sent when PUT `/state` is called.

### POST `/restart`

Restarts the ESP8266, by calling

```ino
ESP.restart();
```
