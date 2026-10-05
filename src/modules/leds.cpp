#include "leds.h"
#include "pins.h"
#include "config.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

LedSystem leds;

static Adafruit_NeoPixel strip(Config::PORTAL_LED_COUNT, Pins::PortalData, NEO_GRB + NEO_KHZ800);

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
                float current = greenFades[i].start_brightness + (greenFades[i].end_brightness - greenFades[i].start_brightness) * progress;
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
    
    // 8-bit resolution = 0-255
    uint32_t duty = (uint32_t)(brightness * 255.0f);
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
