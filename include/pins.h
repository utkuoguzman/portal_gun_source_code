#pragma once
#include <Arduino.h>

namespace Pins {

#ifdef WOKWI_SIM
    // =========================================================================
    // Generic ESP32 DevKitC pins for Wokwi Simulation
    // =========================================================================
    constexpr int GreenLed1   = 12;
    constexpr int GreenLed2   = 14;
    constexpr int GreenLed3   = 27;

    constexpr int PortalData  = 4;  // Top 7-pixel NeoPixel ring data pin

    constexpr int DisplayClk  = 16;
    constexpr int DisplayDio  = 17;

    constexpr int JoystickX   = 34;
    constexpr int JoystickY   = 35;
    constexpr int JoystickSw  = 32;

    constexpr int PowerButton = 33; 

    constexpr int I2sBclk     = 18;
    constexpr int I2sLrc      = 19;
    constexpr int I2sData     = 23;

#else
    // =========================================================================
    // ARDUINO NANO ESP32 — REAL PHYSICAL PINOUT ORDER (USB-C PORT AT TOP)
    // =========================================================================
    //
    //                      [ USB-C PORT ]
    //
    //        LEFT HEADER (Top to Bottom)     RIGHT HEADER (Top to Bottom)
    //        ---------------------------     ----------------------------
    //        D13  (GPIO 48)           |       |   D12  (GPIO 47)
    //        3V3  (3.3V Out)          |       |   D11  (GPIO 38)
    //        B0   (BOOT0 / GPIO 46)   |       |   D10  (GPIO 21)
    //        A0   (GPIO 1)            |       |   D9   (GPIO 18)
    //        A1   (GPIO 2)            |       |   D8   (GPIO 17)
    //        A2   (GPIO 3)            |       |   D7   (GPIO 10)
    //        A3   (GPIO 4)            |       |   D6   (GPIO 9)
    //        A4   (GPIO 11 / SDA)     |       |   D5   (GPIO 8)
    //        A5   (GPIO 12 / SCL)     |       |   D4   (GPIO 7)
    //        A6   (GPIO 13)           |       |   D3   (GPIO 6)
    //        A7   (GPIO 14)           |       |   D2   (GPIO 5)
    //        VBUS (5V USB)            |       |   GND  (Ground)
    //        B1   (BOOT1 / GPIO 0)    |       |   RST  (Reset)
    //        GND  (Ground)            |       |   RX0  (D0 / GPIO 44)
    //        VIN  (6V-21V In)         |       |   TX1  (D1 / GPIO 43)
    //        ------------------------------------------------------------
    //
    // -------------------------------------------------------------------------
    // LEFT HEADER (Top to Bottom, starting from USB-C)
    // -------------------------------------------------------------------------
    // Pin: D13       (GPIO 48) - [Unused / Spare GPIO]
    // Pin: 3V3       - 3.3V Output Rail (Powers Joystick VCC strictly at 3.3V!)
    // Pin: B0        - BOOT0 ROM Trigger (GPIO 46)
    constexpr int JoystickX   = A0;   // Pin: A0  (GPIO 1)  - Joystick Horizontal Potentiometer (VRX)
    constexpr int JoystickY   = A1;   // Pin: A1  (GPIO 2)  - Joystick Vertical Potentiometer (VRY)
    // Pin: A2        (GPIO 3)  - [Unused / Spare Analog or Digital GPIO]
    // Pin: A3        (GPIO 4)  - [Unused / Spare Analog or Digital GPIO]
    // Pin: A4        (GPIO 11) - [Unused / I2C SDA]
    // Pin: A5        (GPIO 12) - [Unused / I2C SCL]
    // Pin: A6        (GPIO 13) - [Unused / Spare Analog]
    // Pin: A7        (GPIO 14) - [Unused / Spare Analog]
    // Pin: VBUS      - 5.0V USB Power Feed
    // Pin: B1        - BOOT1 ROM Trigger (GPIO 0)
    // Pin: GND       - Common Power Ground Bus
    // Pin: VIN       - 2S LiPo Battery Raw Switched (+) Input (6V to 21V into MP2322)

    // -------------------------------------------------------------------------
    // RIGHT HEADER (Top to Bottom, starting from USB-C)
    // -------------------------------------------------------------------------
    constexpr int I2sLrc      = D12;  // Pin: D12 (GPIO 47) - MAX98357A I2S LRC / WS (Word Select)
    constexpr int I2sBclk     = D11;  // Pin: D11 (GPIO 38) - MAX98357A I2S BCLK (Bit Clock)
    constexpr int PowerButton = D10;  // Pin: D10 (GPIO 21) - Soft Power Sleep / Wake (RTC_GPIO21)
    constexpr int JoystickSw  = D9;   // Pin: D9  (GPIO 18) - Joystick Pushbutton (INPUT_PULLUP)
    constexpr int DisplayDio  = D8;   // Pin: D8  (GPIO 17) - TM1637 4-Digit Display DIO
    constexpr int DisplayClk  = D7;   // Pin: D7  (GPIO 10) - TM1637 4-Digit Display CLK
    constexpr int I2sData     = D6;   // Pin: D6  (GPIO 9)  - MAX98357A I2S DIN (Data In)
    constexpr int PortalData  = D5;   // Pin: D5  (GPIO 8)  - Top 7-pixel NeoPixel Ring DIN
    constexpr int GreenLed3   = D4;   // Pin: D4  (GPIO 7)  - Front HUD Green LED 3 (Right, PWM)
    constexpr int GreenLed2   = D3;   // Pin: D3  (GPIO 6)  - Front HUD Green LED 2 (Center, PWM)
    constexpr int GreenLed1   = D2;   // Pin: D2  (GPIO 5)  - Front HUD Green LED 1 (Left, PWM)
    // Pin: GND       - Common Power Ground Bus
    // Pin: RST       - Hardware Reset Pushbutton / Header Pin
    // Pin: RX0 / D0  (GPIO 44) - [Unused / Reserved for Serial RX]
    // Pin: TX1 / D1  (GPIO 43) - [Unused / Reserved for Serial TX]

#endif

} // namespace Pins
