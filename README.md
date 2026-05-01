# ESP32 RC Car Firmware

Firmware for an ESP32-based RC car control/bridge board built with PlatformIO and the Arduino framework.

The current codebase focuses on ESP-NOW communication:

- It connects to Wi‑Fi in station mode with a fixed local IP.
- It accepts command messages over the USB serial port.
- It forwards throttle and steering commands to a target ESP32 using ESP-NOW.
- It can also receive ESP-NOW packets and drive the car outputs through the ESP32 DAC pins.

In practice, this repository acts as the wireless control layer for a custom RC car project.

## Repository layout

- src/main.cpp — main firmware logic, serial parsing, Wi‑Fi setup, and ESP-NOW send/receive handling
- lib/esp_now/espnow.cpp, lib/esp_now/espnow.h — ESP-NOW helper functions, peer management, and message callbacks
- lib/config/pins.h — hardware pin assignments
- lib/config/debug.h — debug logging macros
- platformio.ini — build configuration and compile-time flags
- generate_compile_commands.py — generates compile commands for editor tooling

## Hardware behavior

This firmware uses the ESP32 DAC outputs to generate control signals:

- DAC throttle output: GPIO 25
- DAC steering output: GPIO 26
- Status LED: GPIO 4

When a valid ESP-NOW message is received, the firmware writes the incoming value to the corresponding DAC output.

## Communication flow

1. A command is sent to the board over USB serial.
2. The board parses the destination MAC address and command payload.
3. If needed, the destination peer is added to the ESP-NOW peer list.
4. The board forwards the command over ESP-NOW.
5. On the receiving side, the payload is converted into DAC output for throttle or steering.

## Command format

Serial commands must use the following format:

```text
<MAC>/<command>
```

Where:

- <MAC> is the destination device MAC address as 12 hexadecimal characters with no separators
- <command> is either a throttle or steering command

Supported command payloads:

- t:<value> — throttle value
- s:<value> — steering value

Examples:

```text
64B70895F940/t:150
64B70895F940/s:135
```

The value is parsed as an integer and is typically expected to be in the 0–255 range.

## Requirements

- PlatformIO
- An ESP32 board supported by the Arduino framework
- ESP32 toolchain installed through PlatformIO

The project is currently configured for:

- Board: pico32
- Environment: board1
- Serial monitor speed: 115200

## Setup

1. Clone the repository.
2. Open the folder in VS Code with the PlatformIO extension installed.
3. Review and update the following values in src/main.cpp before flashing:
	- Wi‑Fi SSID
	- Wi‑Fi password
	- Static IP last octet if needed
4. Review platformio.ini if you need a different device ID or hardware target.

Important compile-time options in platformio.ini:

- DEVICE_ID — identifies the board in serial/debug output
- IP_ADDR_LAST_OCTET — last octet of the static local IP
- DEBUG — enables or disables debug prints
- TTR — selects the zero-position calibration values used for steering/throttle
- ESPNOW — enables ESP-NOW support

## Build and upload

Use PlatformIO from the project root:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

If you are using a different board or environment, select the appropriate PlatformIO environment in platformio.ini before building.

## Serial usage

Open the serial monitor at 115200 baud and send one command per line using the format shown above.

The firmware will:

- read the destination MAC address
- add the peer automatically if it is not already known
- send the throttle or steering value over ESP-NOW

## Notes

- The project includes PubSubClient and ArduinoJson in the PlatformIO configuration, but the active code path in this repository uses ESP-NOW.
- The ESP-NOW helper code currently contains a few pre-defined MAC addresses that can be reused for known peers.

## Troubleshooting

- No serial output: confirm the monitor baud rate is 115200.
- ESP-NOW send failures: verify the destination MAC address and make sure the peer has been added successfully.
- Unexpected steering or throttle values: check the command prefix (t: or s:) and confirm the numeric range.


