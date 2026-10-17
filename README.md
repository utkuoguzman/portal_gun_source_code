# 🌌 Rick and Morty Portal Gun (ESP32-S3 / Arduino Nano ESP32)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build%20Passing-brightgreen.svg?logo=platformio)](https://platformio.org/)
[![Hardware](https://img.shields.io/badge/Hardware-Arduino%20Nano%20ESP32%20(ESP32--S3)-blue.svg?logo=arduino)](https://store.arduino.cc/products/nano-esp32)
[![Simulation](https://img.shields.io/badge/Wokwi-Simulation%20Ready-orange.svg)](https://wokwi.com/)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

An ultra screen-accurate, fully interactive **Rick and Morty Portal Gun** firmware and hardware build. Powered by the **ESP32-S3** (Arduino Nano ESP32), this prop features authentic sound effects, high-fidelity I2S digital audio with music soundtracks, real-time universe dialing on a 7-segment display, animated NeoPixel portal rings, phone-based Wi-Fi captive portal color tuning, and **playable 7-segment retro mini-games (Dino Runner & Snake Jazz)** right on the gun!

---

## 📑 Table of Contents
1. [✨ Feature Highlights](#-feature-highlights)
2. [🎮 Controls & Operation](#-controls--operation)
3. [🚀 Special Dimensions & Mini-Games](#-special-dimensions--mini-games)
4. [🛠️ Hardware & Components](#️-hardware--components)
5. [🔌 Wiring Diagram & Pinout](#-wiring-diagram--pinout)
6. [🖨️ 3D Printing Files](#️-3d-printing-files)
7. [💻 Firmware Installation & Build](#-firmware-installation--build)
8. [🧪 Wokwi Browser Simulation](#-wokwi-browser-simulation)

---

## ✨ Feature Highlights

* **🎛️ Physical Universe Dialing**: Dial from Universe `A-001` through `Z-999` using the analog joystick. Defaults to Rick Sanchez's home universe: **`C-137`**.
* **⚡ Firing Portal Beam**: Pressing the joystick button triggers an authentic portal blast (`snd_portal_open1`), illuminating the 7-LED front barrel ring and top fluid cylinder with pulsing interdimensional plasma.
* **🦖 Playable Chrome T-Rex Dino Runner**: Play the iconic offline dinosaur jumping game across the 4 digits of the TM1637 display, complete with the looping official soundtrack *"Jesus Was a Dinosaur"*!
* **🐍 Playable 7-Segment Snake Game**: Full snake game moving across 7-segment display segments with 4-way steering, food spawning, and the official looping **Snake Jazz** track.
* **🎲 Interdimensional Chamber Roulette**: A Russian Roulette gambling minigame with cylinder ratcheting sounds, tense heartbeat LEDs, and an explosive blast followed by Rick gloating: *"Riggity Riggity Wrecked Son!"*.
* **🎙️ Rick Sanchez Voice Soundboard (44 Quotes)**: Authentic voice lines from the show with live speech LED pulses and 4-letter TM1637 subtitles (`PCKL`, `WUBB`, `RIGG`, `bUtr`, etc.).
* **🎵 Evil Morty Theme & The Golden Portal**: Dedicated mode that plays the orchestral *Evil Morty Theme* (For the Damaged Coda) with atmospheric amber breathing pulses.
* **🎉 Party Mode (Get Schwifty)**: Plays *Get Schwifty* with 3-band audio reactive equalizer lighting and synchronized scrolling karaoke lyrics.
* **🌐 Phone Web App & Live Color Tuning**: Spawns an internal Wi-Fi Access Point (`Portal-Gun-C137`) with a captive portal web page. Features a live 360° rainbow hue slider to tune your portal beam color in real-time, plus one-tap character presets (Jerry Green, Morty Yellow, Unity Blue, Summer Pink, Beth Red, Rick White).
* **📡 Citadel & Federation Proximity Radar**: Sweeps the outer barrel LEDs in a rotating sonar beam and uses Bluetooth LE to scan for nearby phones, alerting you to Federation spies (`FDED`) and Rival Ricks (`R1CK`).
* **🔦 High-Power Torch**: Double-click on `'L'` to drive all barrel NeoPixels at 100% pure white (`255, 255, 255`) for a bright cosplay flashlight.
* **🔋 Power Management**: Push-button power switch with deep sleep (sub-milliamp battery preservation) and smooth power-down/power-up sequences.

---

## 🎮 Controls & Operation

```
               [ UP: Increase Letter / Jump ]
                              ▲
                              │
  [ LEFT: Decrement Number ] ◄─┼─► [ RIGHT: Increment Number ]
                              │
                              ▼
             [ DOWN: Decrease Letter / Duck ]

             [ CLICK: Fire Portal Beam / Select ]
```

### 1. Dimension Selection (Idle Screen)
* **UP / DOWN**: Steps through universe dimension letters (`A` through `Z`). *Preserves your dialed number.*
* **RIGHT / LEFT**: Increments or decrements the dimension number (`001` through `999`).
* **1 CLICK**: Fires the portal beam! Barrel NeoPixels and top fluid bulb ignite with pulsing green plasma while the blast sound plays.
* **2 CLICKS (Double-Click)**: Opens the **Special Dimension Mode** corresponding to the selected letter (e.g., `'D'` for Dino Runner, `'S'` for Snake Game, `'U'` for Wi-Fi tuning).
* **3 CLICKS (Triple-Click)**: **Universal Escape Hatch** — Instantly closes any active portal beam, stops any playing music or quote, and returns to Dimension Selection from anywhere!

---

## 🚀 Special Dimensions & Mini-Games

Dial the letter on the TM1637 display and **Double-Click (2 Clicks)** to enter:

| Code | Mode Name | Description | Hidden Cheat Code |
| :---: | :--- | :--- | :--- |
| **`D-003`** | **🦖 Chrome Dino Runner** | Jump over small/tall cacti and duck under low birds! Plays looping *"Jesus Was a Dinosaur"*. Push **UP** to jump, **DOWN** to duck. Displays alive score in seconds (` 38s`). | Dial `D-005`, `D-010`, or `D-099` to start with **5, 10, or 99 lives**! |
| **`S-003`** | **🐍 7-Segment Snake** | Steer the snake across digit segments with 4-way steering while listening to looping **Snake Jazz**! Eats blinking food and grows up to 28 segments. | Dial `S-005` or `S-020` to start with **5 or 20 lives**! |
| **`R-006`** | **🎲 Chamber Roulette** | 6-chamber cylinder. Click to pull the trigger: safe metallic clacks or explosive boom with Rick's *"Riggity Riggity Wrecked Son!"*. | Dial `R-002` (50/50 chance) or `R-010` (10 chambers) to customize odds! |
| **`E-001`** | **🎻 Evil Morty Theme** | Golden portal mode playing the full *For the Damaged Coda* theme with golden amber lighting and scrolling marquee (`EVIL MORTY `). | — |
| **`P-001`** | **🎉 Party Mode** | Plays *Get Schwifty* with audio-reactive EQ lighting and synchronized scrolling lyrics (`SHOW ME WHAT YOU GOT`). | — |
| **`Q-044`** | **🎙️ Rick Quotes (44 Lines)** | Speaks Rick's most iconic lines with talking LEDs. Push joystick in **any direction** (UP/DOWN/LEFT/RIGHT) to sample quotes! | Dial `Q-001` for Pickle Rick (`PCKL`), `Q-005` for Butter Robot, etc. |
| **`U-001`** | **🌐 Web App / Color Tuner** | Spawns `Portal-Gun-C137` Wi-Fi AP. Connect your phone to tune portal beam RGB color live via a 360° hue slider. | — |
| **`B-001`** | **📡 Citadel Radar** | 60-second sonar sweep (`b-60` ... `b-00`) scanning for Bluetooth LE signals with Federation & Rival Rick proximity alerts. | — |
| **`L-001`** | **🔦 High-Power Torch** | 100% brightness pure white flashlight (`LItE`). | — |

---

## 🛠️ Hardware & Components

| Component | Recommendation | Function / Notes |
| :--- | :--- | :--- |
| **Microcontroller** | **Arduino Nano ESP32** (ESP32-S3, 16MB Flash) | Full 16MB flash accommodates all high-res audio tracks, Wi-Fi, BLE, and game logic. |
| **Audio DAC & Amp** | **MAX98357A I2S DAC Amplifier** | Clear, loud 3W digital audio without PWM noise or buzzing. |
| **Speaker** | **4Ω or 8Ω 3W Mini Speaker** (28mm – 40mm) | Mounted in the gun body or handle base for rich sound. |
| **Display** | **TM1637 4-Digit 7-Segment Display** (Red, 0.36") | Displays dimension coordinates, mini-games, and HUD. |
| **Barrel LEDs** | **WS2812B 7-Pixel LED Ring** | Mounted in the front emitter nozzle. |
| **Top Bulb LEDs** | **WS2812B 8-Pixel LED Strip** | Placed inside the top transparent fluid reservoir tube. |
| **Front LEDs** | **3x 3mm or 5mm Green LEDs** + 220Ω Resistors | Screen-accurate front indicator LEDs and physical Life Count HUD. |
| **Joystick** | **Analog Thumb Joystick (KY-023 / ARD-358)** | Dual-axis analog stick with center push-button switch. |
| **Power Button** | **Momentary Push Button** | Connected to `D10` (RTC GPIO) for waking and sleeping. |
| **Battery & Power** | **2S LiPo / Li-Ion (7.4V, 850mAh)** + 5V Buck Regulator (or direct VIN) | Compact 2S pack powering the 5V bus (LEDs/Amp) and Nano ESP32 VIN (6V–21V tolerant). |

> [!CAUTION]
> **Power the Analog Joystick from 3.3V, NOT 5V!**  
> The ESP32-S3 ADC inputs are strictly 3.3V-tolerant. Supplying 5V to the joystick potentiometer will permanently damage the microcontroller ADC pins.

---

## 🔌 Wiring Diagram & Pinout

### Arduino Nano ESP32 (ESP32-S3 Pinout)

```
===================================================================
COMPONENT            PIN ON MODULE       NANO ESP32 PIN    NOTES
===================================================================
Front Green LED 1    Anode (+ 220Ω)      D2                Left HUD LED
Front Green LED 2    Anode (+ 220Ω)      D3                Center HUD LED
Front Green LED 3    Anode (+ 220Ω)      D4                Right HUD LED
-------------------------------------------------------------------
NeoPixel Barrel Ring DIN (Data In)       D5                7x WS2812B
NeoPixel Top Strip   DIN (Data In)       D6                8x WS2812B
-------------------------------------------------------------------
TM1637 Display       CLK (Clock)         D7                4-Digit Red
TM1637 Display       DIO (Data)          D8                4-Digit Red
-------------------------------------------------------------------
Joystick X-Axis      VRX (Horizontal)    A0                Analog ADC
Joystick Y-Axis      VRY (Vertical)      A1                Analog ADC
Joystick Button      SW (Click Switch)   D9                Internal PULLUP
-------------------------------------------------------------------
Power Switch         Momentary Button    D10               RTC Wake / Sleep
-------------------------------------------------------------------
MAX98357A I2S DAC    BCLK (Bit Clock)    D11               I2S Clock
MAX98357A I2S DAC    LRC (Word Select)   D12               I2S Left/Right
MAX98357A I2S DAC    DIN (Data In)       D13               I2S Audio Data
MAX98357A I2S DAC    GAIN                100kΩ to GND      Gain = 9dB
===================================================================
Power Bus            VCC / VIN           5V / 3.3V         Joystick to 3.3V!
Ground Bus           GND                 GND               Common Ground
===================================================================
```

---

## 🖨️ 3D Printing Files

The 3D printable CAD models for the portal gun chassis are organized in the project folder:
* **`Chassis / Main Shell`**: Left & Right clamshell body with battery compartment and internal PCB mounts.
* **`Front Barrel Nozzle`**: Holds the 7-LED NeoPixel ring and front acrylic diffuser.
* **`Top Fluid Cylinder`**: Houses the 8-LED WS2812B strip under a frosted green cylinder tube.
* **`Display Bezel`**: Snug front snap-fit bezel for the 0.36" TM1637 4-digit display module.
* **`Electronics Sled`**: Slide-in mounting tray securing the Arduino Nano ESP32, MAX98357A amplifier, and 2S 850mAh battery compartment.

*(Recommended Print Settings: PLA or PETG, 0.2mm layer height, 15-20% infill, supports required on clamshell handles).*

---

## 💻 Firmware Installation & Build

### Prerequisites
1. Install [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE Extension](https://platformio.org/).
2. Install [Python 3.10+](https://www.python.org/) and ensure it is added to your system `PATH`.
3. (Optional) Install `ffmpeg` if you plan to convert custom audio tracks.

### Step 1: Clone the Repository
```bash
git clone https://github.com/your-username/portal-gun-esp32.git
cd portal-gun-esp32
```

### Step 2: Audio Conversion & Asset Generation
The project includes a Python audio pipeline that converts all audio files to raw 11,025 Hz 8-bit unsigned mono PCM arrays packed into flash memory:
```bash
python tools/generate_extra_audio.py
```
*(You will see all tracks—including Evil Morty, Snake Jazz, and Dino Song—packed into `include/extra_audio.h` and `src/modules/extra_audio.cpp`).*

### Step 3: Build & Flash
Connect your **Arduino Nano ESP32** via USB-C and run:
```bash
pio run -e arduino_nano_esp32 --target upload
```

To monitor debug serial output:
```bash
pio device monitor -b 115200
```

---

## 🧪 Wokwi Browser Simulation

Want to test the firmware before wiring the hardware? You can run the entire portal gun in the [Wokwi](https://wokwi.com/) simulator:

1. Build the simulation target:
   ```bash
   pio run -e esp32dev
   ```
   *(PlatformIO automatically merges the bootloader, partition table, and firmware into `.pio/build/esp32dev/firmware.merged.bin`)*.
2. In VS Code, install the **Wokwi for VS Code** extension.
3. Open `diagram.json` and press **F1** $\rightarrow$ `Wokwi: Start Simulator`.
4. Interact with the virtual joystick, 7-segment display, NeoPixels, and buzzer directly in your browser!

---

## 📜 License & Open Source

This project (firmware, schematics, and 3D CAD files) is released under the **[GNU General Public License v3.0 (GPL-3.0)](LICENSE)** — the exact same copyleft license used by **Blender** and **Prusa Research** because I used them:

* **Freedom to Use & Modify**: You are completely free to build, run, modify, and improve this portal gun for your own projects and cosplay.
* **Share-Alike Requirement**: If you modify, redistribute, or release your own versions (firmware, hardware mods, or remix 3D files), you **must share your modifications as open source under the same GNU GPL v3 license**.
* **Disclaimer**: Rick and Morty, character names, and audio clips are trademarks and copyright of Adult Swim / Warner Bros. Discovery. This is an open-source non-commercial fan tribute prop project.

---

> *"Wubba Lubba Dub Dub! Grab your portal gun and let's go, Morty!"* 🥒
