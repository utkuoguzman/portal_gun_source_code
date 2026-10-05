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
            for(int i=0; i<3; i++) leds.setGreen(i, 1.0f);
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
    
    uint8_t low_vol = 0;
    uint8_t mid_vol = 0;
    uint8_t high_vol = 0;
    
    if (env_getschwifty_low_size > 0 && env_getschwifty_mid_size > 0 && env_getschwifty_high_size > 0) {
        size_t envIdx = posMs / 100; // 10 samples per second
        if (envIdx < env_getschwifty_low_size) low_vol = env_getschwifty_low[envIdx];
        if (envIdx < env_getschwifty_mid_size) mid_vol = env_getschwifty_mid[envIdx];
        if (envIdx < env_getschwifty_high_size) high_vol = env_getschwifty_high[envIdx];
    } else {
        // Fake envelope if no script output yet
        uint8_t dummy = 127 + (sin(posMs / 150.0f) * 127.0f);
        low_vol = dummy; mid_vol = dummy; high_vol = dummy;
    }
    
    // Multiply by 4 to make the visualizer much brighter, since the extracted envelopes are relatively quiet
    float l_val = (low_vol > 5) ? (low_vol * 4.0f / 255.0f) : 0.0f;
    float m_val = (mid_vol > 5) ? (mid_vol * 4.0f / 255.0f) : 0.0f;
    float h_val = (high_vol > 5) ? (high_vol * 4.0f / 255.0f) : 0.0f;
    
    if (l_val > 1.0f) l_val = 1.0f;
    if (m_val > 1.0f) m_val = 1.0f;
    if (h_val > 1.0f) h_val = 1.0f;

    // LEDs react as a 3-bar spectrogram
    leds.setGreen(0, l_val);
    leds.setGreen(1, m_val);
    leds.setGreen(2, h_val);
    
    // Portal ring pulses mainly to bass
    leds.setPortalBrightness(l_val);
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        int hue = (i * 255 / Config::PORTAL_LED_COUNT + millis() / 10) % 255;
        // simplified spectrum
        if (hue < 85) leds.setPortalColor(i, hue * 3, 255 - hue * 3, 0);
        else if (hue < 170) leds.setPortalColor(i, 255 - (hue-85) * 3, 0, (hue-85) * 3);
        else leds.setPortalColor(i, 0, (hue-170) * 3, 255 - (hue-170) * 3);
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
    
    esp_sleep_enable_ext0_wakeup((gpio_num_t)Pins::PowerButton, 0); 
    esp_deep_sleep_start();
}
