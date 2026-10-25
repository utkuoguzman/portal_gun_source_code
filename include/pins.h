#pragma once
#include <Arduino.h>

namespace Pins {

#if defined(ARDUINO_NANO_ESP32)
    // =========================================================================
    // ARDUINO NANO ESP32 (ESP32-S3)
    // =========================================================================
    constexpr int JoystickX   = A0;   // Pin: A0  (GPIO 1)
    constexpr int JoystickY   = A1;   // Pin: A1  (GPIO 2)
    constexpr int I2sLrc      = D12;  // Pin: D12 (GPIO 47)
    constexpr int I2sBclk     = D11;  // Pin: D11 (GPIO 38)
    constexpr int PowerButton = D10;  // Pin: D10 (GPIO 21)
    constexpr int JoystickSw  = D9;   // Pin: D9  (GPIO 18)
    constexpr int DisplayDio  = D8;   // Pin: D8  (GPIO 17)
    constexpr int DisplayClk  = D7;   // Pin: D7  (GPIO 10)
    constexpr int I2sData     = D6;   // Pin: D6  (GPIO 9)
    constexpr int PortalData  = D5;   // Pin: D5  (GPIO 8)
    constexpr int GreenLed3   = D4;   // Pin: D4  (GPIO 7)
    constexpr int GreenLed2   = D3;   // Pin: D3  (GPIO 6)
    constexpr int GreenLed1   = D2;   // Pin: D2  (GPIO 5)

#else
    // =========================================================================
    // ESP32 DEVKIT V1 (30-PIN DUAL-CORE ESP-WROOM-32)
    // Grouped by function: Left Header = UI/Inputs | Right Header = Audio & LEDs
    // =========================================================================
    //
    //                      [ ESP-WROOM-32 ANTENNA (TOP) ]
    //
    //        LEFT HEADER (Inputs & Display)     RIGHT HEADER (Audio & LEDs)
    //        ------------------------------     ---------------------------
    //  [15]: EN   (Hardware Reset Button)|      | [15]: D23  (GPIO 23) -> LED_HUD3 (Right)
    //  [14]: VP   (GPIO 36 / Spare)      |      | [14]: D22  (GPIO 22) -> LED_HUD2 (Center)
    //  [13]: VN   (GPIO 39 / Spare)      |      | [13]: TX0  (GPIO 1  / USB TX)
    //  [12]: D34  (GPIO 34) -> JOY_VRX   |      | [12]: RX0  (GPIO 3  / USB RX)
    //  [11]: D35  (GPIO 35) -> JOY_VRY   |      | [11]: D21  (GPIO 21) -> LED_HUD1 (Left)
    //  [10]: D32  (GPIO 32) -> JOY_SW    |      | [10]: D19  (GPIO 19) -> NEO_RING_DIN
    //  [ 9]: D33  (GPIO 33) -> BTN_WAKE  |      | [ 9]: D18  (GPIO 18) -> I2S_BCLK
    //  [ 8]: D25  (GPIO 25) -> DISP_CLK  |      | [ 8]: D5   (GPIO 5  / Spare)
    //  [ 7]: D26  (GPIO 26) -> DISP_DIO  |      | [ 7]: TX2  (GPIO 17) -> I2S_LRC
    //  [ 6]: D27  (GPIO 27 / Spare)      |      | [ 6]: RX2  (GPIO 16) -> I2S_DIN
    //  [ 5]: D14  (GPIO 14 / Spare)      |      | [ 5]: D4   (GPIO 4  / Spare)
    //  [ 4]: D12  (GPIO 12 / Strapping)  |      | [ 4]: D2   (GPIO 2  / Blue LED)
    //  [ 3]: D13  (GPIO 13 / Spare)      |      | [ 3]: D15  (GPIO 15 / Strapping)
    //  [ 2]: GND  (Ground Return Bus)    |      | [ 2]: GND  (Ground Return Bus)
    //  [ 1]: VIN  (+5V Safe Power In)    |      | [ 1]: 3V3  (+3.3V Out for Joy Pots)
    //        ------------------------------------------------------------
    //                      [ MICRO-USB / USB-C (BOTTOM) ]
    //
    // --- User Controls & Display (Left Header: Pins 7 to 12) ---
    constexpr int JoystickX   = 34;   // Pin 12: D34 (GPIO 34 / ADC1_CH6) - Horizontal Pot (VRX)
    constexpr int JoystickY   = 35;   // Pin 11: D35 (GPIO 35 / ADC1_CH7) - Vertical Pot (VRY)
    constexpr int JoystickSw  = 32;   // Pin 10: D32 (GPIO 32) - Pushbutton (INPUT_PULLUP)
    constexpr int PowerButton = 33;   // Pin 9:  D33 (GPIO 33 / RTC_GPIO8) - Soft Power Wake Button
    constexpr int DisplayClk  = 25;   // Pin 8:  D25 (GPIO 25) - TM1637 Display Clock (CLK)
    constexpr int DisplayDio  = 26;   // Pin 7:  D26 (GPIO 26) - TM1637 Display Data (DIO)

    // --- Audio & Lighting Actuators (Right Header: Pins 6, 7, 9, 10, 11, 14, 15) ---
    constexpr int GreenLed1   = 21;   // Pin 11: D21 (GPIO 21) - Left Green LED PWM (via 220Ω resistor)
    constexpr int GreenLed2   = 22;   // Pin 14: D22 (GPIO 22) - Center Green LED PWM (via 220Ω resistor)
    constexpr int GreenLed3   = 23;   // Pin 15: D23 (GPIO 23) - Right Green LED PWM (via 220Ω resistor)
    constexpr int PortalData  = 19;   // Pin 10: D19 (GPIO 19) - Top 7-Pixel WS2812B Ring DIN (via 330Ω resistor)
    constexpr int I2sBclk     = 18;   // Pin 9:  D18 (GPIO 18) - MAX98357A I2S Bit Clock (BCLK)
    constexpr int I2sLrc      = 17;   // Pin 7:  TX2 (GPIO 17) - MAX98357A Word Select (LRC/WS)
    constexpr int I2sData     = 16;   // Pin 6:  RX2 (GPIO 16) - MAX98357A Serial Data In (DIN)

#endif

} // namespace Pins
