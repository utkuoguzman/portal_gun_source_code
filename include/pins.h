#pragma once

namespace Pins {
    constexpr int GreenLed1 = 25;
    constexpr int GreenLed2 = 26;
    constexpr int GreenLed3 = 27;

    constexpr int PortalData = 4;

    constexpr int DisplayClk = 16;
    constexpr int DisplayDio = 17;

    constexpr int JoystickX = 34; // ADC1
    constexpr int JoystickY = 35; // ADC1
    constexpr int JoystickSw = 32;

    constexpr int PowerButton = 33; // RTC GPIO for wake up

    constexpr int I2sBclk = 18;
    constexpr int I2sLrc  = 19;
    constexpr int I2sData = 23;
}
