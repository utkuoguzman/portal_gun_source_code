#include "portal_gun.h"
#include "input.h"
#include "leds.h"
#include "display.h"
#include "audio.h"
#include "audio_assets.h"
#include "config.h"
#include "pins.h"
#include <Arduino.h>
#include <esp_sleep.h>

PortalGun portalGun;

static char dimLetter = 'C';
static int dimNumber = 137;
static unsigned long portalOpenTime = 0;
static bool portalIsOpen = false;
static bool portalJustClosed = false;
static float frontLedSmooth[3] = {0.0f, 0.0f, 0.0f};

void PortalGun::begin() {
    // Start at DeepSleep or PowerUp based on wakeup cause
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        changeState(PortalState::PowerUp);
    } else {
        // Normal boot (plugged in), we can go to sleep immediately or power up.
        // Let's power up for testing.
        changeState(PortalState::PowerUp);
    }
}

void PortalGun::update() {
    const auto& joy = input.getState();
    
    // Global Power Button override (if not already powering down/sleeping)
    if (joy.powerJustPressed && currentState != PortalState::PowerDown && currentState != PortalState::DeepSleep) {
        changeState(PortalState::PowerDown);
    }

    switch (currentState) {
        case PortalState::PowerUp:
            handlePowerUp();
            break;
        case PortalState::PowerUpFinalize:
            handlePowerUpFinalize();
            break;
        case PortalState::DimensionSelection:
            handleDimensionSelection();
            break;
        case PortalState::Party:
            handleParty();
            break;
        case PortalState::PowerDown:
            handlePowerDown();
            break;
        case PortalState::DeepSleep:
            handleDeepSleep();
            break;
        default:
            break;
    }
}

void PortalGun::changeState(PortalState newState) {
    currentState = newState;
    stateStartTime = millis();
    stateInit = true;
    
    Serial.print("State changed to: ");
    switch (currentState) {
        case PortalState::PowerUp: Serial.println("PowerUp"); break;
        case PortalState::PowerUpFinalize: Serial.println("PowerUpFinalize"); break;
        case PortalState::DimensionSelection: Serial.println("DimensionSelection"); break;
        case PortalState::Party: Serial.println("Party"); break;
        case PortalState::PowerDown: Serial.println("PowerDown"); break;
        case PortalState::DeepSleep: Serial.println("DeepSleep"); break;
    }
}

void PortalGun::handlePowerUp() {
    uint32_t elapsed = millis() - stateStartTime;
    const uint32_t duration = 4000;
    
    if (stateInit) {
        stateInit = false;
        audio.playRaw(snd_powerup, snd_powerup_size);
        display.clear();
        for(int i=0; i<3; i++) leds.setGreen(i, 0.0f);
        leds.clearPortal();
    }
    
    // 1) 4-Digit Display Loading Animation (Empty -> I -> II -> II I -> II II ...)
    // segments e and f make a vertical bar on the left ('I'), b and c make one on the right.
    int step = (elapsed * 9) / duration; // 0 to 8
    uint8_t dispData[4] = {0};
    
    for(int i=0; i<4; i++) {
        if (step >= i*2 + 2) {
            dispData[i] = 0x36; // II
        } else if (step == i*2 + 1) {
            dispData[i] = 0x30; // I
        } else {
            dispData[i] = 0x00; // empty
        }
    }
    display.showSegments(dispData);

    // 2) 3 front lights animation
    // light 1, then 1+2, then 1+2+3, then rapid blink all.
    if (elapsed < 1000) {
        leds.setGreen(0, 1.0f); leds.setGreen(1, 0.0f); leds.setGreen(2, 0.0f);
    } else if (elapsed < 2000) {
        leds.setGreen(0, 1.0f); leds.setGreen(1, 1.0f); leds.setGreen(2, 0.0f);
    } else if (elapsed < 3000) {
        leds.setGreen(0, 1.0f); leds.setGreen(1, 1.0f); leds.setGreen(2, 1.0f);
    } else {
        // Rapid flash all
        if ((elapsed / 100) % 2 == 0) {
            for(int i=0; i<3; i++) leds.setGreen(i, 1.0f);
        } else {
            for(int i=0; i<3; i++) leds.setGreen(i, 0.0f);
        }
    }

    if (elapsed >= duration) {
        changeState(PortalState::PowerUpFinalize);
    }
}

void PortalGun::handlePowerUpFinalize() {
    uint32_t elapsed = millis() - stateStartTime;
    
    if (stateInit) {
        stateInit = false;
        audio.playRaw(snd_buckleup, snd_buckleup_size);
        for(int i=0; i<3; i++) leds.setGreen(i, 0.0f);
        
        // Calculate exact scroll speed to match audio
        uint32_t audioLengthMs = (snd_buckleup_size > 0) ? (snd_buckleup_size * 1000) / 11025 : 3000;
        if (audioLengthMs == 0) audioLengthMs = 3000;
        
        // "BUCKLE UP BITCHES " is 18 chars. Total steps = 18 + 4 = 22.
        int speed = audioLengthMs / 22; 
        display.startScrolling("BUCKLE UP BITCHES ", speed);
    }
    
    // Wait until BOTH the audio and the scroll are finished
    bool audioFinished = (snd_buckleup_size > 0) ? !audio.isPlaying() : (elapsed > 3000);
    
    if (audioFinished && display.isScrollFinished()) {
        changeState(PortalState::DimensionSelection);
    }
}

void PortalGun::handleDimensionSelection() {
    uint32_t elapsed = millis() - stateStartTime;
    const auto& joy = input.getState();
    
    if (stateInit) {
        stateInit = false;
        portalIsOpen = false;
        portalJustClosed = false;
        leds.clearPortal();
    }
    
    // Party transition
    if (joy.doubleClicked) {
        changeState(PortalState::Party);
        return;
    }

    if (portalIsOpen) {
        // Open logic
        if (joy.justPressed || millis() - portalOpenTime >= 5000) {
            portalIsOpen = false;
            portalJustClosed = true;
            // Play random close sound
            if (millis() % 2 == 0) {
                audio.playRaw(snd_portal_close1, snd_portal_close1_size);
            } else {
                audio.playRaw(snd_portal_close2, snd_portal_close2_size);
            }
            leds.clearPortal();
            for(int i=0; i<3; i++) leds.setGreen(i, 0.0f);
        } else {
            // Portal animation
            float pulse = (sin(millis() / 200.0f) + 1.0f) / 2.0f;
            leds.setPortalBrightness(0.2f + 0.1f * pulse);
            for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
                leds.setPortalColor(i, 0, 255, 0);
            }
            leds.showPortal();
            float frontPulse = 0.75f + 0.25f * pulse;
            for(int i=0; i<3; i++) leds.setGreen(i, frontPulse);
        }
    } else {
        // Update dimension letter (Up/Down)
        if (joy.justPressed) {
            if (!portalJustClosed) {
                // Open portal
                portalIsOpen = true;
                portalOpenTime = millis();
                if (millis() % 2 == 0) {
                    audio.playRaw(snd_portal_open1, snd_portal_open1_size);
                } else {
                    audio.playRaw(snd_portal_open2, snd_portal_open2_size);
                }
            } else {
                portalJustClosed = false;
            }
        }
        
        static unsigned long lastMove = 0;
        if (millis() - lastMove > 200) {
            if (joy.up) {
                dimLetter++;
                if (dimLetter > 'Z') dimLetter = 'A';
                lastMove = millis();
            } else if (joy.down) {
                dimLetter--;
                if (dimLetter < 'A') dimLetter = 'Z';
                lastMove = millis();
            }
            
            if (joy.right) {
                dimNumber++;
                if (dimNumber > 999) dimNumber = 0;
                lastMove = millis();
            } else if (joy.left) {
                dimNumber--;
                if (dimNumber < 0) dimNumber = 999;
                lastMove = millis();
            }
        }

        // Display C137 formatting
        char buf[5];
        snprintf(buf, sizeof(buf), "%c%03d", dimLetter, dimNumber);
        display.showText(buf);
    }
}

void PortalGun::handleParty() {
    const auto& joy = input.getState();
    uint32_t elapsed = millis() - stateStartTime;
    
    if (stateInit) {
        stateInit = false;
        currentLyricIndex = 0;
        isLyricScrolling = false;
        frontLedSmooth[0] = 0.0f;
        frontLedSmooth[1] = 0.0f;
        frontLedSmooth[2] = 0.0f;
        audio.playRaw(snd_getschwifty, snd_getschwifty_size);
    }
    
    if (joy.doubleClicked) {
        audio.stop();
        changeState(PortalState::DimensionSelection);
        return;
    }

    if (!audio.isPlaying() && elapsed > 1000) {
        changeState(PortalState::DimensionSelection);
        return;
    }

    // Party visuals based on 3-band envelope
    uint32_t posMs = audio.getPositionMs();
    
    float low_val = 0.0f;
    float mid_val = 0.0f;
    float high_val = 0.0f;
    
    if (env_getschwifty_low_size > 0 && env_getschwifty_mid_size > 0 && env_getschwifty_high_size > 0) {
        size_t envIdx = posMs / 100; // 10 samples per second
        float frac = (posMs % 100) / 100.0f;
        
        auto interp = [](const uint8_t* arr, size_t sz, size_t idx, float f) -> float {
            if (idx >= sz) return 0.0f;
            float v1 = arr[idx];
            float v2 = (idx + 1 < sz) ? arr[idx + 1] : v1;
            return v1 + (v2 - v1) * f;
        };

        low_val  = interp(env_getschwifty_low, env_getschwifty_low_size, envIdx, frac);
        mid_val  = interp(env_getschwifty_mid, env_getschwifty_mid_size, envIdx, frac);
        high_val = interp(env_getschwifty_high, env_getschwifty_high_size, envIdx, frac);
    } else {
        // Fake envelope if no script output yet
        float dummy = 127.0f + (sinf(posMs / 150.0f) * 127.0f);
        low_val = dummy; mid_val = dummy; high_val = dummy;
    }
    
    // Scale visualizer to full dynamic range (0.0 to 1.0)
    float l_val = (low_val > 5.0f) ? (low_val * 4.0f / 255.0f) : 0.0f;
    float m_val = (mid_val > 5.0f) ? (mid_val * 4.0f / 255.0f) : 0.0f;
    float h_val = (high_val > 5.0f) ? (high_val * 4.0f / 255.0f) : 0.0f;
    
    if (l_val > 1.0f) l_val = 1.0f;
    if (m_val > 1.0f) m_val = 1.0f;
    if (h_val > 1.0f) h_val = 1.0f;

    // Dynamic contrast curves (makes beats punch while keeping quiet passages subtle)
    float targets[3] = {
        powf(l_val, 1.3f), // Low/Bass
        powf(m_val, 1.3f), // Mid/Vocals
        powf(h_val, 1.3f)  // High/Treble
    };

    // Analog VU-meter ballistics: fast attack on drum/vocal hits, smooth exponential decay
    for (int k = 0; k < 3; k++) {
        if (targets[k] > frontLedSmooth[k]) {
            // Fast attack: jump up to catch transient peaks
            frontLedSmooth[k] += (targets[k] - frontLedSmooth[k]) * 0.85f;
        } else {
            // Smooth exponential decay: gradual, organic falloff
            frontLedSmooth[k] += (targets[k] - frontLedSmooth[k]) * 0.18f;
        }
        leds.setGreen(k, frontLedSmooth[k]);
    }
    
    // Portal ring operates at upper brightness values (85% to 100% of max brightness)
    float total_energy = (l_val + m_val + h_val) / 3.0f;
    float party_brightness = Config::PORTAL_MAX_BRIGHTNESS * (0.85f + 0.15f * total_energy);
    leds.setPortalBrightness(party_brightness);

    // Color/Hue changes according to frequencies: Low = Blue, Mid = Green, High = Red
    float r_weight = powf(h_val, 1.8f); // High -> Red
    float g_weight = powf(m_val, 1.8f); // Mid  -> Green
    float b_weight = powf(l_val, 1.8f); // Low  -> Blue

    float max_weight = fmaxf(r_weight, fmaxf(g_weight, b_weight));
    uint8_t pr = 0, pg = 0, pb = 0;

    if (max_weight > 0.0001f) {
        // Normalize so dominant frequency is fully saturated at upper brightness
        pr = (uint8_t)((r_weight / max_weight) * 255.0f);
        pg = (uint8_t)((g_weight / max_weight) * 255.0f);
        pb = (uint8_t)((b_weight / max_weight) * 255.0f);
    } else {
        // Fallback default if quiet (portal green)
        pr = 0; pg = 255; pb = 0;
    }

    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        leds.setPortalColor(i, pr, pg, pb);
    }
    leds.showPortal();
    
    // Subtitle Sync for 4-Digit Display
    if (currentLyricIndex < getschwifty_lyrics_count) {
        uint32_t startMs = getschwifty_lyrics[currentLyricIndex].startTimeMs;
        uint32_t endMs = getschwifty_lyrics[currentLyricIndex].endTimeMs;
        
        if (posMs >= startMs && posMs <= endMs) {
            if (!isLyricScrolling) {
                const char* text = getschwifty_lyrics[currentLyricIndex].text;
                uint32_t duration = endMs - startMs;
                int steps = strlen(text) + 4;
                int speed = duration / steps;
                if (speed == 0) speed = 100;
                
                display.startScrolling(text, speed);
                isLyricScrolling = true;
            }
        }
        
        if (posMs > endMs) {
            currentLyricIndex++;
            isLyricScrolling = false;
            display.clear();
        }
    } else {
        if (!isLyricScrolling) {
            display.clear(); // Empty display if no lyrics are currently active
        }
    }
}

void PortalGun::handlePowerDown() {
    uint32_t elapsed = millis() - stateStartTime;
    const uint32_t duration = 4000;
    
    if (stateInit) {
        stateInit = false;
        audio.playRaw(snd_powerdown, snd_powerdown_size);
        leds.clearPortal();
    }
    
    // 1) 4-Digit Display Power Down (II II II II -> ... -> Empty)
    int step = 8 - ((elapsed * 9) / duration); // 8 to 0
    if (step < 0) step = 0;
    
    uint8_t dispData[4] = {0};
    for(int i=0; i<4; i++) {
        if (step >= i*2 + 2) {
            dispData[i] = 0x36; // II
        } else if (step == i*2 + 1) {
            dispData[i] = 0x30; // I
        } else {
            dispData[i] = 0x00; // empty
        }
    }
    display.showSegments(dispData);

    // 2) 3 front lights reverse animation
    if (elapsed < 1000) {
        // rapid blink
        if ((elapsed / 100) % 2 == 0) {
            for(int i=0; i<3; i++) leds.setGreen(i, 1.0f);
        } else {
            for(int i=0; i<3; i++) leds.setGreen(i, 0.0f);
        }
    } else if (elapsed < 2000) {
        leds.setGreen(0, 1.0f); leds.setGreen(1, 1.0f); leds.setGreen(2, 1.0f);
    } else if (elapsed < 3000) {
        leds.setGreen(0, 1.0f); leds.setGreen(1, 1.0f); leds.setGreen(2, 0.0f);
    } else {
        leds.setGreen(0, 1.0f); leds.setGreen(1, 0.0f); leds.setGreen(2, 0.0f);
    }

    if (elapsed >= duration) {
        changeState(PortalState::DeepSleep);
    }
}

void PortalGun::handleDeepSleep() {
    display.clear();
    for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
    leds.clearPortal();
    
    Serial.println("Entering Deep Sleep...");
    
    esp_sleep_enable_ext0_wakeup((gpio_num_t)digitalPinToGPIONumber(Pins::PowerButton), 0); 
    esp_deep_sleep_start();
}
