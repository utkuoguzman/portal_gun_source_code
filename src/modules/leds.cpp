#include "leds.h"
#include "pins.h"
#include "config.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

LedSystem leds;

static Adafruit_NeoPixel strip(Config::PORTAL_LED_COUNT, Pins::PortalData, NEO_GRB + NEO_KHZ800);
static bool torchActive = false;

void LedSystem::begin() {
    // Setup green LED PWM channels
    // Using ledc framework for ESP32
    ledcSetup(0, 5000, 8); // Channel 0, 5 kHz, 8-bit resolution
    ledcSetup(1, 5000, 8);
    ledcSetup(2, 5000, 8);
    
    ledcAttachPin(Pins::GreenLed1, 0);
    ledcAttachPin(Pins::GreenLed2, 1);
    ledcAttachPin(Pins::GreenLed3, 2);

    setGreen(0, 0.0f);
    setGreen(1, 0.0f);
    setGreen(2, 0.0f);

    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    strip.setBrightness(255); // We handle brightness manually
}

void LedSystem::update() {
    if (torchActive) return;
    uint32_t now = millis();
    
    // Update green LED animations
    for (int i = 0; i < 3; i++) {
        if (greenFades[i].active) {
            uint32_t elapsed = now - greenFades[i].start_time;
            if (elapsed >= greenFades[i].duration) {
                if (greenFades[i].flash) {
                    setGreen(i, greenFades[i].return_brightness);
                } else {
                    setGreen(i, greenFades[i].end_brightness);
                }
                greenFades[i].active = false;
            } else {
                float progress = (float)elapsed / (float)greenFades[i].duration;
                if (progress > 1.0f) progress = 1.0f;
                // SmoothStep S-curve easing for natural acceleration and deceleration
                float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
                float current = greenFades[i].start_brightness + (greenFades[i].end_brightness - greenFades[i].start_brightness) * smoothProgress;
                setGreen(i, current);
            }
        }
    }
}

void LedSystem::setGreen(int channel, float brightness) {
    if (channel < 0 || channel > 2) return;
    
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;
    
    currentGreen[channel] = brightness;
    
    // Perceptual Gamma 2.2 curve (CIE 1931 approximation)
    // Matches the non-linear logarithmic sensitivity of the human eye
    float gammaDuty = (brightness > 0.0f) ? powf(brightness, 2.2f) : 0.0f;
    uint32_t duty = (uint32_t)(gammaDuty * 255.0f + 0.5f);
    if (duty > 255) duty = 255;
    ledcWrite(channel, duty);
}

void LedSystem::fadeGreen(int channel, float from, float to, uint32_t duration) {
    if (channel < 0 || channel > 2) return;
    
    greenFades[channel].active = true;
    greenFades[channel].start_brightness = from;
    greenFades[channel].end_brightness = to;
    greenFades[channel].start_time = millis();
    greenFades[channel].duration = duration;
    greenFades[channel].flash = false;
}

void LedSystem::flashGreen(int channel, float brightness, uint32_t duration) {
    if (channel < 0 || channel > 2) return;
    
    greenFades[channel].active = true;
    greenFades[channel].start_brightness = brightness;
    greenFades[channel].end_brightness = brightness; // hold at this brightness
    greenFades[channel].start_time = millis();
    greenFades[channel].duration = duration;
    greenFades[channel].flash = true;
    greenFades[channel].return_brightness = currentGreen[channel];
    
    setGreen(channel, brightness);
}

void LedSystem::setPortalColor(int index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < 0 || index >= Config::PORTAL_LED_COUNT) return;
    
    // Apply global brightness limiter
    r = (uint8_t)((float)r * globalPortalBrightness);
    g = (uint8_t)((float)g * globalPortalBrightness);
    b = (uint8_t)((float)b * globalPortalBrightness);
    
    strip.setPixelColor(index, strip.Color(r, g, b));
}

void LedSystem::setAllPortalColor(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

void LedSystem::setTopColor(uint8_t r, uint8_t g, uint8_t b) {
    (void)r; (void)g; (void)b;
}

void LedSystem::setCustomPortalColor(uint8_t r, uint8_t g, uint8_t b) {
    customR = r;
    customG = g;
    customB = b;
    hasCustomColor = true;
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

void LedSystem::getCustomColor(uint8_t &r, uint8_t &g, uint8_t &b) const {
    r = customR;
    g = customG;
    b = customB;
}

void LedSystem::setPortalBrightness(float brightness) {
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > Config::PORTAL_MAX_BRIGHTNESS) brightness = Config::PORTAL_MAX_BRIGHTNESS;
    
    globalPortalBrightness = brightness;
}

void LedSystem::showPortal() {
    strip.show();
}

void LedSystem::clearPortal() {
    strip.clear();
    strip.show();
}

void LedSystem::setTorchMode(bool enable) {
    torchActive = enable;
    if (enable) {
        // High-power pure white on all 7 portal LEDs
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            strip.setPixelColor(i, strip.Color(255, 255, 255));
        }
        strip.show();
        for (int i = 0; i < 3; i++) {
            setGreen(i, 0.6f);
        }
    } else {
        clearPortal();
        for (int i = 0; i < 3; i++) {
            setGreen(i, 0.0f);
        }
    }
}

void LedSystem::setRadarSweep(int headPixel, bool alert) {
    if (alert) {
        // Rapid flashing red/amber alert
        uint8_t r = ((millis() / 150) % 2 == 0) ? 255 : 0;
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            strip.setPixelColor(i, strip.Color(r, 0, 0));
        }
        strip.show();
        for (int i = 0; i < 3; i++) {
            setGreen(i, (r > 0) ? 1.0f : 0.0f);
        }
        return;
    }

    // Normal radar sweep:
    // Pixel 0 (center): gentle pulsing sonar beacon
    float centerPulse = (sinf(millis() / 250.0f) + 1.0f) * 0.5f;
    uint8_t cG = (uint8_t)(50 + 150 * centerPulse);
    strip.setPixelColor(0, strip.Color(0, cG, 0));

    // Pixels 1 to 6 (outer ring): rotating radar beam with a decaying trail
    int ringSize = (Config::PORTAL_LED_COUNT > 1) ? (Config::PORTAL_LED_COUNT - 1) : 1;
    int head = (headPixel % ringSize) + 1;
    int trail1 = ((headPixel - 1 + ringSize) % ringSize) + 1;
    int trail2 = ((headPixel - 2 + ringSize) % ringSize) + 1;

    for (int i = 1; i < Config::PORTAL_LED_COUNT; i++) {
        if (i == head) {
            strip.setPixelColor(i, strip.Color(0, 255, 0)); // Bright leading beam
        } else if (i == trail1) {
            strip.setPixelColor(i, strip.Color(0, 90, 0));  // Trail 1
        } else if (i == trail2) {
            strip.setPixelColor(i, strip.Color(0, 25, 0));  // Trail 2
        } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }
    strip.show();

    // Front LEDs gentle instrument sweep
    for (int i = 0; i < 3; i++) {
        float fVal = (sinf((millis() / 200.0f) + (i * 1.2f)) + 1.0f) * 0.35f;
        setGreen(i, fVal);
    }
}
