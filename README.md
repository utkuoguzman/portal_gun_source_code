# Portal Gun ESP32 Project

This repository contains the firmware and development files for an ESP32-based Portal Gun prop.

## Features (Milestone 1)
- State machine-based animations
- Generic ESP32 target
- Wokwi simulation configuration
- 3x green LED PWM control
- WS2812 portal effect simulation
- 4-digit TM1637 display
- Analog joystick input

## Building

Requires VS Code with PlatformIO and the Wokwi extension.

```bash
pio run
```

## Simulation

Open `diagram.json` in VS Code and start the Wokwi simulation.
The simulation will run `.pio/build/esp32dev/firmware.bin` automatically.
