#pragma once
#include <Arduino.h>

namespace Pins {
#ifdef WOKWI_SIM
    // Generic ESP32 DevKitC pins for Wokwi simulation
    constexpr int GreenLed1 = 25;
    constexpr int GreenLed2 = 26;
    constexpr int GreenLed3 = 27;

    constexpr int PortalData = 4;
    constexpr int TopData = 5; // Added top strip pin

    constexpr int DisplayClk = 16;
    constexpr int DisplayDio = 17;

    constexpr int JoystickX = 34;
    constexpr int JoystickY = 35;
    constexpr int JoystickSw = 32;

    constexpr int PowerButton = 33; 

    constexpr int I2sBclk = 18;
    constexpr int I2sLrc  = 19;
    constexpr int I2sData = 23;
#else
    // Arduino Nano ESP32 standard D-pins and A-pins
    constexpr int GreenLed1 = D2;
    constexpr int GreenLed2 = D3;
    constexpr int GreenLed3 = D4;

    constexpr int PortalData = D5;
    constexpr int TopData = D6;

    constexpr int DisplayClk = D7;
    constexpr int DisplayDio = D8;

    constexpr int JoystickX = A0; 
    constexpr int JoystickY = A1; 
    constexpr int JoystickSw = D9;

    constexpr int PowerButton = D10; 

    constexpr int I2sBclk = D11;
    constexpr int I2sLrc  = D12;
    constexpr int I2sData = D13;
#endif
}
