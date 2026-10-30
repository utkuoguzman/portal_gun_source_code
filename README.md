# 🌌 Rick and Morty Portal Gun (ESP32 DevKit V1 / Arduino Nano ESP32)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build%20Passing-brightgreen.svg?logo=platformio)](https://platformio.org/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32%20DevKit%20V1%20%7C%20Nano%20ESP32-blue.svg?logo=espressif)](https://www.espressif.com/)
[![Simulation](https://img.shields.io/badge/Wokwi-Simulation%20Ready-orange.svg)](https://wokwi.com/)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

An ultra screen-accurate, fully interactive **Rick and Morty Portal Gun** firmware and hardware build. Powered by the **ESP32 DevKit V1 (30-Pin ESP-WROOM-32, 4MB Flash)** (with cross-target support for the **Arduino Nano ESP32**), this prop features real-time 4-bit IMA ADPCM I2S digital audio with full music soundtracks and 44 Rick quotes, universe dialing on a 4-digit 7-segment display, animated NeoPixel portal rings, high-power MOSFET-driven 5V green HUD LEDs, phone-based Wi-Fi captive portal control & wireless OTA updates, Bluetooth phone-pairing distance tracking, and **playable 7-segment retro mini-games (Dino Runner & Snake Jazz)** right on the gun!

---

## 📑 Table of Contents
1. [✨ Feature Highlights](#-feature-highlights)
2. [🎮 Controls & Operation](#-controls--operation)
3. [🚀 Special Dimensions & Mini-Games](#-special-dimensions--mini-games)
4. [🛠️ Hardware & Components](#️-hardware--components)
5. [🔌 Wiring Diagram](#-wiring-diagram)
6. [🖨️ 3D Printing Files](#️-3d-printing-files)
7. [💻 Firmware Installation, 4MB ADPCM Pipeline & OTA](#-firmware-installation-4mb-adpcm-pipeline--ota)
8. [🧪 Wokwi Browser Simulation](#-wokwi-browser-simulation)

---

## ✨ Feature Highlights

* **🎛️ Physical Universe Dialing**: Dial from Universe `A-000` through `Z-999` using the analog thumbstick. Defaults to Rick Sanchez's home universe: **`C-137`**.
  * **LEFT / RIGHT**: Changes the **Dimension Letter** (`A`–`Z`). Automatically sets the life counter to **`005` (5 lives)** whenever you dial onto **`S`** (Snake) or **`D`** (Dino) so you don't accidentally start with 137 lives!
  * **UP / DOWN**: Increments or decrements the **Dimension Number** (`000`–`999`).
* **⚡ Firing Portal Beam**: Single-clicking the joystick button triggers an authentic portal blast (`snd_portal_open1` / `snd_portal_open2`), illuminating the 7-LED top fluid bulb ring and the 3 front green HUD LEDs with pulsing interdimensional plasma.
* **🦖 Playable Chrome T-Rex Dino Runner (`D` Dimension)**: Play the offline dinosaur runner across the 4 digits of the TM1637 display with the looping soundtrack *"Jesus Was a Dinosaur"*:
  * **Single-Row Obstacle Variety**: Features ground (`d`), mid-level (`g`), and upper-level (`a`) single-row obstacles (no 2-row tall blockades). The **2nd obstacle** is always a mid-level obstacle so players immediately see height variety.
  * **Progressive Speed Curve**: Shifts at a steady `520 ms` pace for the first 2 obstacles, then progressively accelerates from the **3rd obstacle onward** down to `190 ms` per step.
  * **Extended Jump Arc & Live Score**: Jump duration scales automatically (`~2.85` landscape shifts) with current game speed, and displays your survival score (`XXs`) every time you lose a life and on Game Over.
* **🐍 Playable 7-Segment Snake Game (`S` Dimension)**: Full snake game moving across 7-segment display segments at a smooth `440 ms` base step with 4-way steering, blinking food, live 3-LED life counter HUD, score display (`SCxx`) after every life lost, and the looping **Snake Jazz** track.
* **🎲 Interdimensional Chamber Roulette (`R` / `G` Dimension)**: Russian Roulette minigame with mechanical cylinder ratcheting sounds, live chamber LED HUD, safe metallic clacks, or an explosive blast followed by Rick gloating: *"Riggity Riggity Wrecked Son!"*.
* **🎙️ Rick Sanchez Voice Soundboard (`Q` Dimension — 44 Quotes)**: Authentic voice lines from the show with live speech LED pulses and 4-letter TM1637 subtitles (`PCKL`, `WUBB`, `RIGG`, `bUtr`, etc.). Push the joystick in any direction to sample another quote.
* **🎵 Evil Morty Theme & The Golden Portal (`E` Dimension)**: Plays *For the Damaged Coda* with atmospheric golden-amber breathing pulses and scrolling `EVIL MORTY ` marquee.
* **🎉 Party Mode (`P` Dimension — Get Schwifty)**: Plays *Get Schwifty* with 3-band audio-reactive equalizer lighting and synchronized scrolling karaoke lyrics.
* **🌐 Phone Web App, Live Color Tuning & Wireless OTA (`U` Dimension)**: Spawns an open Wi-Fi Access Point (`Portal-Gun-C137` at `http://192.168.4.1`) with a Captive Portal control panel (Fire/Close Portal, Dimension Dialer, RGB Color Picker, Soundboard, Party Mode, Torch, and Wireless OTA Firmware Update at `/update`).
* **📡 Bluetooth `R1CK` Distance Tracker (`B` Dimension)**:
  * Broadcasts **`Portal-Gun-C137`** via both Classic Bluetooth (with Extended Inquiry Response) and BLE, waiting up to **5 minutes** on `PAIR` for your phone to pair (using passwordless Simple Secure Pairing *"Just Works"*).
  * Once paired, locks onto your phone and displays **`R1CK`**: as you bring your phone closer, the **`R1CK`** screen on/off blink accelerates (`650 ms` down to `65 ms`) and the Portal LEDs shift smoothly from **Pure Green (far) → Yellow/Amber (mid) → Pure Red (close)**.
* **🔦 High-Power Torch (`L` Dimension)**: Drives the 7-pixel NeoPixel ring at 100% pure white (`255, 255, 255`) (`LItE`).
* **🔋 Dual-Trigger Deep Sleep & Power Management**: Trigger the animated `PowerDown` sequence and ultra-low-power Deep Sleep by **pressing the `D33` Sleep Button** OR **holding the `D32` Joystick Button for 3 seconds**. Internal ESP32 pull-downs (`gpio_pulldown_en` + `gpio_deep_sleep_hold_en`) lock the `2N7000` MOSFET gates `LOW` during boot and sleep (no external $100\text{k}\Omega$ pull-down resistors needed), and RTC pull-ups (`rtc_gpio_pullup_en`) keep `D33` stable in Deep Sleep.

---

## 🎮 Controls & Operation

*(Configured for a KY-023 thumbstick mounted with its 5-pin header pointing UP at 12 o'clock)*:

```
                [ UP: Increment Number / Dino Jump ]
                                 ▲
                                 │
   [ LEFT: Previous Letter ] ◄───┼───► [ RIGHT: Next Letter ]
                                 │
                                 ▼
               [ DOWN: Decrement Number / Dino Duck ]

             [ 1x CLICK: Fire Portal Beam ]
             [ 2x CLICK: Enter Special Dimension / Game ]
             [ 3x CLICK: Universal Escape Back to Dialer ]
             [ 3s HOLD (or D33 Button): Power Down / Deep Sleep ]
```

---

## 🚀 Special Dimensions & Mini-Games

Dial the letter on the TM1637 display using **LEFT / RIGHT** and **Double-Click (2 Clicks)** to enter:

| Code | Mode Name | Description | Custom Number Dialing |
| :---: | :--- | :--- | :--- |
| **`D-005`** | **🦖 Chrome Dino Runner** | Jump over ground (`d`) obstacles and avoid mid-level (`g`) & upper-level (`a`) obstacles! Push **UP** to jump, **DOWN** to duck. Shows score (`XXs`) after each life lost. | Auto-snaps to **`005` (5 lives)** when dialing onto `D`. Use UP/DOWN to set `1`–`99` lives! |
| **`S-005`** | **🐍 7-Segment Snake** | Steer the snake across digit segments with 4-way steering while listening to looping **Snake Jazz**. Shows score (`SCxx`) after each life lost. | Auto-snaps to **`005` (5 lives)** when dialing onto `S`. Use UP/DOWN to set `1`–`99` lives! |
| **`R-006`** | **🎲 Chamber Roulette** | 6-chamber cylinder (`G` also supported as alias). Click to pull the trigger: safe metallic clacks or explosive boom with *"Riggity Riggity Wrecked Son!"*. | Dial `R-002` (50/50 odds) up to `R-099` to customize cylinder size! |
| **`E-001`** | **🎻 Evil Morty Theme** | Golden portal mode playing *For the Damaged Coda* with golden-amber lighting and scrolling `EVIL MORTY ` marquee. | — |
| **`P-001`** | **🎉 Party Mode** | Plays *Get Schwifty* with 3-band audio-reactive EQ lighting and synchronized scrolling lyrics (`SHOW ME WHAT YOU GOT`). | — |
| **`Q-044`** | **🎙️ Rick Quotes (44 Lines)** | Speaks Rick's iconic lines with talking LEDs. Push joystick **UP/DOWN/LEFT/RIGHT** to sample random quotes! | Dial `Q-001` through `Q-044` to start on a specific quote (`1` = Pickle Rick). |
| **`U-001`** | **🌐 Wi-Fi Web Portal & OTA** | Spawns `Portal-Gun-C137` Wi-Fi AP (`120s` countdown `U120`..`U-00`). Connect phone to `http://192.168.4.1` for remote control & `/update` for OTA. | — |
| **`B-001`** | **📡 Bluetooth `R1CK` Tracker** | Waits on `PAIR` (up to 5 min) for your phone to pair with **`Portal-Gun-C137`**, then blinks **`R1CK`** faster and shifts LEDs **Green → Yellow → Red** with proximity. | Single-click joystick on `PAIR` to skip straight to distance tracking if already paired. |
| **`L-001`** | **🔦 High-Power Torch** | 100% brightness pure white flashlight (`LItE`). | — |

---

## 🛠️ Hardware & Components

| Component | Specification | Function / Notes |
| :--- | :--- | :--- |
| **Microcontroller** | **ESP32 DevKit V1** (30-Pin ESP-WROOM-32, 4MB Flash) | Primary target (`esp32dev`). Also supports **Arduino Nano ESP32** (`arduino_nano_esp32`). |
| **Audio DAC & Amp** | **MAX98357A I2S Class-D Amplifier** | Clear, loud 3W digital audio with hardware-matched `-1.5 dBFS` anti-clipping headroom. |
| **Speaker** | **4Ω or 8Ω 3W Mini Speaker** (28mm – 40mm) | Connected directly to MAX98357A differential `SPK+` / `SPK-` outputs. |
| **Display** | **TM1637 4-Digit 7-Segment Display** (Red, 0.36") | Displays dimension coordinates, scrolling marquees, mini-games, and scores. |
| **Top Bulb LEDs** | **WS2812B 7-Pixel NeoPixel Ring** (+ `330Ω` DIN resistor) | Placed inside top fluid bulb. |
| **Front HUD LEDs** | **3x Green LEDs** + **3x `2N7000` N-MOSFETs** (`TO-92`) | Driven from the `+5V` rail via `100Ω` current-limiting resistors and `1kΩ` gate resistors for maximum brightness without loading ESP32 GPIOs. |
| **Joystick** | **KY-023 Dual-Axis Analog Thumbstick** | Mounted with 5-pin header pointing **UP**; powered strictly from **`3.3V`**. |
| **Sleep/Wake Button** | **Momentary Push Button** | Connected between `D33` (`GPIO 33`) and `GND` (uses internal RTC pull-up). |
| **Battery & Power** | **2S Li-Ion / LiPo (7.4V–8.4V)** + **5V 3A Buck Converter** | Regulated `+5.0V` rail feeds ESP32 `VIN` (`5V`), MAX98357A, WS2812B Ring, TM1637, and MOSFET LED anodes. |

> [!CAUTION]
> **Power the KY-023 Analog Joystick from `3.3V`, NOT `5V`!**  
> The ESP32 ADC pins (`D34` and `D35`) are strictly `3.3V`-tolerant. Connecting joystick `+5V` to the `3.3V` rail ensures the `VRX`/`VRY` wipers never exceed `3.3V`.

---

## 🔌 Wiring Diagram

### Standard Engineering Schematic (EasyEDA / KiCad Net-Standard)
![Rick & Morty Portal Gun EasyEDA Schematic Diagram](./easyeda_schematic_diagram.png)
* *Vector SVG*: [`easyeda_schematic_diagram.svg`](./easyeda_schematic_diagram.svg)

---

## 🖨️ 3D Printing Files

The complete 3D CAD model for the Portal Gun is located in the [`3D_model/`](./3D_model/) directory:
* **[`3D_model/Portal_Gun_v2.1.blend`](./3D_model/Portal_Gun_v2.1.blend)**: Native Blender project containing all printable Portal Gun parts (main housing, front emitter nozzle, top fluid cylinder, and display mount).
* *(Recommended Print Settings: PLA or PETG, `0.2mm` layer height, `15–20%` infill).*

---

## 💻 Firmware Installation, 4MB ADPCM Pipeline & OTA

### Dual-Bank OTA on 4MB Flash (`partitions_portal.csv`)
To fit all **56 audio tracks** (Get Schwifty, Evil Morty, Dino Song, Snake Jazz, Power-Up/Down, Portal Blasts, and 44 Rick Sanchez quotes), the Web Captive Portal assets, Wi-Fi stack, and VHCI Bluetooth stack into a **4MB ESP32** while preserving **fail-safe Dual-Bank Wireless OTA (`app0` + `app1` at `1.93 MB` each)**:
* Audio assets in `src/modules/audio_assets.cpp`, `src/modules/extra_audio.cpp`, and `src/modules/soundboard_data.cpp` are encoded in **8,400 Hz 4-bit IMA ADPCM** (`2` samples per byte) and decoded in real time by `AudioSystem` (`src/modules/audio.cpp`).
* Bluetooth proximity & pairing (`src/modules/radar.cpp`) talks directly to the ESP32's built-in Bluetooth controller over **raw VHCI HCI commands**, avoiding the `~950 KB` Bluedroid C++ library overhead.
* Original uncompressed 16MB assets are backed up in `backup_16mb_assets/`.

### Build & Flash via USB (`COM5`)
```powershell
& "$HOME\.platformio\penv\Scripts\platformio.exe" run -e esp32dev -t upload --upload-port COM5
```

### Wireless Over-The-Air (OTA) Updates
1. Dial to **`U`** on the Portal Gun and **double-click** the joystick button (`U120` countdown starts).
2. Connect your phone or PC to the **`Portal-Gun-C137`** Wi-Fi network.
3. Open **`http://192.168.4.1/update`** in your browser and upload `.pio/build/esp32dev/firmware.bin` (or run `pio run -e esp32dev_ota -t upload`).

---

## 🧪 Wokwi Browser Simulation

1. Build the project with `pio run -e esp32dev`.
2. Open `diagram.json` in VS Code with the **Wokwi for VS Code** extension installed and run `Wokwi: Start Simulator`.

---

## 📜 License & Open Source

This project is released under the **[GNU General Public License v3.0 (GPL-3.0)](LICENSE)**.
* **Disclaimer**: Rick and Morty, character names, and audio clips are trademarks and copyright of Adult Swim / Warner Bros. Discovery. This is an open-source non-commercial fan tribute prop project.

---

> *"Wubba Lubba Dub Dub! Grab your portal gun and let's go, Morty!"* 🥒
