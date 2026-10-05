#pragma once
#include <stdint.h>

class LedSystem {
public:
    void begin();
    void update();

    void setGreen(int channel, float brightness); // channel 0-2, brightness 0.0-1.0
    void fadeGreen(int channel, float from, float to, uint32_t duration);
    void flashGreen(int channel, float brightness, uint32_t duration);

    void setPortalColor(int index, uint8_t r, uint8_t g, uint8_t b);
    void setPortalBrightness(float brightness); // 0.0-1.0
    void showPortal();
    void clearPortal();
    
private:
    // Animation state for green LEDs
    struct FadeState {
        bool active = false;
        float start_brightness = 0.0f;
        float end_brightness = 0.0f;
        uint32_t start_time = 0;
        uint32_t duration = 0;
        
        bool flash = false; // if true, returns to previous brightness after duration
        float return_brightness = 0.0f;
    };
    FadeState greenFades[3];
    float currentGreen[3] = {0.0f, 0.0f, 0.0f};
    
    float globalPortalBrightness = 0.0f;
};

extern LedSystem leds;
