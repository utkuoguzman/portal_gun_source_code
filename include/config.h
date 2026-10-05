#pragma once
#include <stdint.h>

namespace Config {
    constexpr float JOYSTICK_DEADZONE = 0.12f;
    
    constexpr float GREEN_IDLE_BRIGHTNESS = 0.20f;
    constexpr float PORTAL_IDLE_BRIGHTNESS = 0.12f;
    constexpr float PORTAL_MAX_BRIGHTNESS = 0.35f;
    
    constexpr uint32_t FIRE_FLASH_MS = 120;
    
    constexpr float DEFAULT_VOLUME = 0.70f;
    
    constexpr int PORTAL_LED_COUNT = 20;
    
    constexpr int AUDIO_SAMPLE_RATE = 22050;

    constexpr int JOYSTICK_X_CENTER = 2048;
    constexpr int JOYSTICK_Y_CENTER = 2048;
}
