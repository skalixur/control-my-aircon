# Control My Aircon

Turn a cheap ESP8266 or ESP32 and an IR LED into a Home Assistant aircon controller. It works with any of the ~80 aircon brands [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) supports, and it stays in sync when someone uses the physical remote.

Flash it, join its WiFi from your phone, press a button on your remote. It shows up in Home Assistant by itself.

## What you get in Home Assistant

One device with:

- **A thermostat:** mode, target temperature, fan speed, vertical and horizontal swing, on/off.
- **Voice control:** "Hey Google, turn on the aircon" resumes whatever the **Power-On Mode** setting says: the last-used mode, or always Cool, Heat and so on. Turning it off remembers the mode for next time. Fan speeds and swing positions have plain names, so "set the fan to High" works.
- **Feature switches:** Quiet Mode, Turbo Mode, Eco Mode, Sleep Mode, Display Light. Air Filter, Self-Clean and Beep Sounds are there too, disabled by default because few units support them.
- **A Remote event:** fires whenever someone uses the physical remote, so you can build automations on it.
- **Settings:** Power-On Mode, IR Protocol (dropdown), IR Model, Temperature Unit, Send Delay, "Learn From Remote", "Open Setup Portal", "Restart".
- **Diagnostics:** WiFi signal, IP address, uptime.
- **A "Visit" link** on the device page that opens the controller's own local control page. That page keeps working if Home Assistant is down.

## Setup

You need Home Assistant with the MQTT integration and the Mosquitto broker add-on. Most installs already have them.

1. **Install the libraries** (Arduino IDE → Library Manager):
   - ArduinoJson 7.x
   - IRremoteESP8266 2.8.6+
   - PubSubClient3 3.x
   - WiFiManager 2.0.17+ (by tzapu)

   You also need the ESP8266 or ESP32 board package.

   **ESP32 note:** IRremoteESP8266 2.8.6 doesn't build on ESP32 board package 3.x. Either install IRremoteESP8266 from its [GitHub master branch](https://github.com/crankyoldgit/IRremoteESP8266) (Sketch → Include Library → Add .ZIP Library), or use ESP32 board package 2.0.x. ESP8266 is unaffected.
2. **Flash it.** Open `control-my-aircon.ino`, pick your board (e.g. *NodeMCU 1.0*), and upload. On ESP32, choose a partition scheme with OTA and at least 1.3 MB per app slot (e.g. *Minimal SPIFFS*).
3. **Join the device's WiFi.** Connect your phone to **Aircon Setup xxxx**. Choose your WiFi, give the device a name and room, and enter an MQTT login (any Home Assistant user works with the Mosquitto add-on). Leave *MQTT server* blank and the device finds the broker itself.
4. **Teach it your remote.** Point your aircon remote at the device and press any button. The protocol is learned and saved. If you already know the protocol (e.g. `COOLIX`), type it into the portal's *Aircon protocol* field and skip this step.

Home Assistant discovers the device straight away. Nothing needs editing in the code. If you want to skip the portal or change pins, copy `user_config.example.h` to `user_config.h`.

## Day to day

- **The physical remote still works.** The device sees the remote and updates Home Assistant within a moment. Turn on *Repeat remote commands* if the IR LED is better placed than you are.
- **Local control:** open `http://aircon-xxxxxx.local` (or the IP) for a control page that needs nothing but the device.
- **Updates:** upload a new `.bin` at `http://aircon-xxxxxx.local/update`, or flash over WiFi from the Arduino IDE (the device appears as a network port).
- **FLASH / BOOT button:** tap it to learn the remote again. Hold it 5 seconds to factory reset.
- **Status LED (ESP8266):**
  - Slow blink: setup portal is open.
  - Double blink: connecting to WiFi or MQTT.
  - Fast blink: waiting for the remote.
  - Off: all good.
  - Short flicker: IR sent or received.

## Security

The setup portal and the local page are meant for your home network.

- **Local page and updates:** anyone on your LAN can use the local page and upload firmware unless you set `AIRCON_OTA_PASSWORD` in `user_config.h`. Setting it is recommended.
- **MQTT password:** the setup portal never shows the saved password. Leave the field blank to keep it.

## Hardware

- An ESP8266 (e.g. NodeMCU) or an ESP32
- *n* × 940 nm IR LEDs
- *n* × 60–220 Ω resistors (depends on the LEDs' current rating and your supply voltage; calculate this yourself)
- 1 × 10 kΩ resistor
- 1 × BJT transistor to drive the LEDs, for more range (optional)
- 1 × IR receiver: VS1838B, or a [TSOP](https://github.com/crankyoldgit/IRremoteESP8266/wiki/Frequently-Asked-Questions#Help_Im_getting_very_inconsistent_results_when_capturing_an_IR_message_using_a_VS1838b_IR_demodulator), which is more reliable

Default pins:

| | ESP8266 | ESP32 |
|---|---|---|
| IR LED / transistor | D1 | GPIO 4 |
| IR receiver | D2 | GPIO 14 |

Change them in `user_config.h`. The ESP32 defaults suit the classic ESP32 and the S3. On an ESP32-C3, pick other pins (GPIO 14 is used by flash there) and set `AIRCON_BUTTON_PIN` to 9.

<img width="896" height="462" alt="Wiring diagram" src="https://github.com/user-attachments/assets/859c85bf-13ab-4b83-9320-cddcd3bd351e" />

## Troubleshooting

- **The device doesn't appear in Home Assistant.** Check the serial monitor (115200 baud). It logs every step. A message about the MQTT username and password means the login was rejected. Hold FLASH for 5 s, or press *Open setup portal* in Home Assistant, to fix it.
- **No broker found.** Enter your Home Assistant's IP address as the MQTT server in the setup portal. Use the IP, not `homeassistant.local`: the ESP8266 can't resolve `.local` names for MQTT.
- **The remote isn't learned.** Make sure the receiver is wired to the right pin and the remote is for an aircon. TV-style remotes are ignored on purpose. "ignoring X remote" in the log means a different protocol was already learned. Press *Learn from remote* first.
- **The aircon doesn't respond.** Try a different *IR model* number in Home Assistant. Some brands have several, and the remote can't always tell them apart.

## Upgrading from v1

v1 used the topics `<location>/ac/...` and one discovery message per entity. Set **Room** in the setup portal to your old `location` (e.g. `Bedroom` for `bedroom`). The first time it connects, the device deletes the old entities and the old retained commands, so you don't end up with a ghost device. Your old `config.h` is no longer used and can be deleted.
