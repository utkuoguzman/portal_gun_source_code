#include "portal_gun.h"
#include "input.h"
#include "leds.h"
#include "display.h"
#include "audio.h"
#include "audio_assets.h"
#include "config.h"
#include "pins.h"
#include "radar.h"
#include "wifi_portal.h"
#include "extra_audio.h"
#include "soundboard_data.h"
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
        return;
    }

    // Universal 3-click escape: Return to Dimension Selection from ANY mode, game, torch, party, or firing portal!
    if (joy.tripleClicked) {
        if (currentState != PortalState::DimensionSelection &&
            currentState != PortalState::PowerUp &&
            currentState != PortalState::PowerUpFinalize &&
            currentState != PortalState::PowerDown &&
            currentState != PortalState::DeepSleep) {

            if (currentState == PortalState::WifiPortal) {
                wifiPortal.stopPortal();
            }
            if (currentState == PortalState::RadarScan) {
                radar.stopScan();
            }
            if (currentState == PortalState::Torch) {
                leds.setTorchMode(false);
            }
            audio.stop();
            audio.playRaw(snd_portal_close1, snd_portal_close1_size);
            leds.clearPortal();
            for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
            portalIsOpen = false;
            portalJustClosed = false;
            isQuoteActive = false;
            quoteAudioStarted = false;
            input.clearClicks();
            changeState(PortalState::DimensionSelection);
            return;
        } else if (currentState == PortalState::DimensionSelection && (portalIsOpen || isQuoteActive)) {
            // Portal firing state or Rick quote: 3 clicks immediately closes the portal beam
            audio.stop();
            audio.playRaw(snd_portal_close1, snd_portal_close1_size);
            leds.clearPortal();
            for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
            portalIsOpen = false;
            portalJustClosed = true;
            isQuoteActive = false;
            quoteAudioStarted = false;
            input.clearClicks();
            return;
        }
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
        case PortalState::Torch:
            handleTorch();
            break;
        case PortalState::RadarScan:
            handleRadarScan();
            break;
        case PortalState::WifiPortal:
            handleWifiPortal();
            break;
        case PortalState::EvilMorty:
            handleEvilMorty();
            break;
        case PortalState::SnakeGame:
            handleSnakeGame();
            break;
        case PortalState::Roulette:
            handleRoulette();
            break;
        case PortalState::DinoGame:
            handleDinoGame();
            break;
        case PortalState::RickQuote:
            handleRickQuote();
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
        case PortalState::Torch: Serial.println("Torch"); break;
        case PortalState::RadarScan: Serial.println("RadarScan"); break;
        case PortalState::WifiPortal: Serial.println("WifiPortal"); break;
        case PortalState::EvilMorty: Serial.println("EvilMorty"); break;
        case PortalState::SnakeGame: Serial.println("SnakeGame"); break;
        case PortalState::Roulette: Serial.println("Roulette"); break;
        case PortalState::DinoGame: Serial.println("DinoGame"); break;
        case PortalState::RickQuote: Serial.println("RickQuote"); break;
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
    
    if (portalIsOpen) {
        // Portal beam open: closes automatically after 5000ms timeout (or early via 3 clicks in update())
        if (millis() - portalOpenTime >= 5000) {
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
            float pulse = (sinf(millis() / 200.0f) + 1.0f) * 0.5f;
            leds.setPortalBrightness(0.2f + 0.1f * pulse);
            uint8_t pr = 0, pg = 255, pb = 0;
            if (leds.hasCustomPortalColor()) {
                leds.getCustomColor(pr, pg, pb);
            }
            for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
                leds.setPortalColor(i, pr, pg, pb);
            }
            leds.showPortal();
            float frontPulse = 0.75f + 0.25f * pulse;
            for(int i=0; i<3; i++) leds.setGreen(i, frontPulse);
        }
    } else {
        // Portal is closed:
        // 2 Clicks: Enters universe corresponding to dialed dimension
        if (joy.doubleClicked) {
            input.clearClicks();
            if (dimLetter == 'B') {
                // 'B' dimension activates 60s Citadel/Federation Proximity Radar (BT-Radar)
                changeState(PortalState::RadarScan);
                return;
            } else if (dimLetter == 'U') {
                // 'U' dimension activates 120s Wi-Fi Captive Portal Hotspot (U for URL)
                changeState(PortalState::WifiPortal);
                return;
            } else if (dimLetter == 'P') {
                // 'P' dimension activates Party Mode (Get Schwifty!)
                changeState(PortalState::Party);
                return;
            } else if (dimLetter == 'E') {
                // 'E' dimension activates Evil Morty Theme & Amber Golden Portal
                changeState(PortalState::EvilMorty);
                return;
            } else if (dimLetter == 'S') {
                // 'S' dimension activates 7-Segment Snake Game
                changeState(PortalState::SnakeGame);
                return;
            } else if (dimLetter == 'R' || dimLetter == 'G') {
                // 'R' dimension activates Interdimensional Chamber Roulette (G retained as alias)
                changeState(PortalState::Roulette);
                return;
            } else if (dimLetter == 'D') {
                // 'D' dimension activates Google Chrome Dinosaur Runner
                changeState(PortalState::DinoGame);
                return;
            } else if (dimLetter == 'Q') {
                // 'Q' dimension activates Rick Quotes soundboard browser
                changeState(PortalState::RickQuote);
                return;
            } else if (dimLetter == 'L') {
                // 'L' dimension activates High-Power Torch / Light (LItE)
                changeState(PortalState::Torch);
                return;
            } else {
                // Any other dimension (e.g. C-137 Home), 2 clicks opens portal beam
                portalIsOpen = true;
                portalOpenTime = millis();
                if (millis() % 2 == 0) {
                    audio.playRaw(snd_portal_open1, snd_portal_open1_size);
                } else {
                    audio.playRaw(snd_portal_open2, snd_portal_open2_size);
                }
            }
        } else if (joy.singleClicked) {
            // 1 Click: "open simply a portal wherever we are"
            // Regardless of whether dialed to D, S, R, Q, E, P, U, B, L, or C,
            // 1 click simply opens the portal beam to that dimension!
            input.clearClicks();
            portalIsOpen = true;
            portalOpenTime = millis();
            if (millis() % 2 == 0) {
                audio.playRaw(snd_portal_open1, snd_portal_open1_size);
            } else {
                audio.playRaw(snd_portal_open2, snd_portal_open2_size);
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
    
    // Exit party mode on 3-click emergency exit (or when audio finishes)
    if (joy.tripleClicked) {
        input.clearClicks();
        audio.stop();
        leds.clearPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
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

void PortalGun::handleTorch() {
    const auto& joy = input.getState();
    
    if (stateInit) {
        stateInit = false;
        leds.setTorchMode(true);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 255, 255, 255);
        }
        leds.showPortal();
        display.showText("LItE");
    }

    // Exit Torch mode: 3 clicks gets us out
    if (joy.tripleClicked) {
        input.clearClicks();
        leds.setTorchMode(false);
        leds.clearPortal();
        changeState(PortalState::DimensionSelection);
        return;
    }
}

void PortalGun::handleRadarScan() {
    const auto& joy = input.getState();
    uint32_t elapsed = millis() - stateStartTime;
    int remSec = 60 - (int)(elapsed / 1000);

    if (stateInit) {
        stateInit = false;
        radar.startScan();
        audio.playRaw(snd_portal_open1, snd_portal_open1_size);
    }

    // Finish scan at 60s or if user 3-clicks early
    if (remSec <= 0 || joy.tripleClicked) {
        input.clearClicks();
        radar.stopScan();
        audio.playRaw(snd_portal_close1, snd_portal_close1_size);
        leds.clearPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
        changeState(PortalState::DimensionSelection);
        return;
    }

    // Check for proximity alert
    if (radar.isAlertActive()) {
        display.showText(radar.getAlertText());
        leds.setRadarSweep(0, true); // Alert mode (flashes red/amber)
    } else {
        // Display countdown: b-60, b-59 ... b-00
        char buf[5];
        snprintf(buf, sizeof(buf), "b-%02d", remSec);
        display.showText(buf);

        // Circular sweep: 120ms per step around outer ring
        int step = (elapsed / 120) % 6;
        leds.setRadarSweep(step, false);
    }
}

void PortalGun::handleWifiPortal() {
    const auto& joy = input.getState();
    uint32_t elapsed = millis() - stateStartTime;
    int remSec = 120 - (int)(elapsed / 1000);

    if (stateInit) {
        stateInit = false;
        wifiPortal.startPortal();
        audio.playRaw(snd_portal_open1, snd_portal_open1_size);
    }

    // Finish portal at 120s or if user 3-clicks early
    if (remSec <= 0 || joy.tripleClicked) {
        input.clearClicks();
        wifiPortal.stopPortal();
        audio.playRaw(snd_portal_close1, snd_portal_close1_size);
        leds.clearPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
        changeState(PortalState::DimensionSelection);
        return;
    }

    // Display: countdown U120 down to U000 (U for URL, displays cleanly as 0x3E)
    char buf[5];
    if (remSec > 99) {
        snprintf(buf, sizeof(buf), "U%03d", remSec);
    } else {
        snprintf(buf, sizeof(buf), "U-%02d", remSec);
    }
    display.showText(buf);

    // Visuals: glowing cyan-green interdimensional Wi-Fi pulse
    float wave = (sinf(millis() / 250.0f) + 1.0f) * 0.5f;
    uint8_t pg = 180 + (uint8_t)(75.0f * wave);
    uint8_t pb = (uint8_t)(220.0f * (1.0f - wave));
    leds.setPortalBrightness(0.3f);
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        leds.setPortalColor(i, 0, pg, pb);
    }
    leds.showPortal();

    for (int i = 0; i < 3; i++) {
        leds.setGreen(i, 0.3f + 0.7f * wave);
    }
}

void PortalGun::triggerWebFire() {
    audio.playRaw(snd_portal_open1, snd_portal_open1_size);
    leds.setPortalBrightness(0.35f);
    uint8_t pr = 0, pg = 255, pb = 0;
    if (leds.hasCustomPortalColor()) {
        leds.getCustomColor(pr, pg, pb);
    }
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        leds.setPortalColor(i, pr, pg, pb);
    }
    leds.showPortal();
    for (int i = 0; i < 3; i++) leds.setGreen(i, 1.0f);
}

void PortalGun::triggerWebClose() {
    audio.playRaw(snd_portal_close1, snd_portal_close1_size);
    leds.clearPortal();
    for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
}

void PortalGun::triggerWebQuote() {
    if (rick_soundboard_count > 0) {
        int idx = random(rick_soundboard_count);
        audio.playRaw(rick_soundboard[idx].pcmData, rick_soundboard[idx].size);
        display.showText(rick_soundboard[idx].label);
    }
}

void PortalGun::triggerWebTorch() {
    leds.setTorchMode(true);
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        leds.setPortalColor(i, 255, 255, 255);
    }
    leds.showPortal();
}

void PortalGun::triggerWebParty() {
    changeState(PortalState::Party);
}

void PortalGun::setWebDimension(char letter, int number) {
    dimLetter = letter;
    dimNumber = number;
    char buf[5];
    snprintf(buf, sizeof(buf), "%c%03d", dimLetter, dimNumber);
    display.showText(buf);
}

void PortalGun::setWebColor(uint8_t r, uint8_t g, uint8_t b) {
    leds.setCustomPortalColor(r, g, b);
}

// -------------------------------------------------------------
// FEATURE 1: Evil Morty & The Golden Portal ('E' Dimension)
// -------------------------------------------------------------
void PortalGun::handleEvilMorty() {
    const auto& joy = input.getState();
    uint32_t elapsed = millis() - stateStartTime;
    
    if (stateInit) {
        stateInit = false;
        audio.playRaw(snd_evil_morty, snd_evil_morty_size);
        display.startScrolling("EVIL MORTY ", 220);
    }
    
    // Stop early on 3 clicks or when the song ends
    if (joy.tripleClicked || !audio.isPlaying()) {
        input.clearClicks();
        audio.stop();
        audio.playRaw(snd_portal_close1, snd_portal_close1_size);
        leds.clearPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
        changeState(PortalState::DimensionSelection);
        return;
    }
    
    // Continuous loop scrolling of "EVIL MORTY "
    if (display.isScrollFinished()) {
        display.startScrolling("EVIL MORTY ", 220);
    }
    
    // Atmospheric golden amber breathing pulse
    float pulse = (sinf(millis() / 260.0f) + 1.0f) * 0.5f;
    uint8_t amberR = 255;
    uint8_t amberG = 160 + (uint8_t)(35.0f * pulse);
    uint8_t amberB = 0;
    
    leds.setPortalBrightness(0.35f + 0.15f * pulse);
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        leds.setPortalColor(i, amberR, amberG, amberB);
    }
    leds.showPortal();
    
    for (int i = 0; i < 3; i++) {
        leds.setGreen(i, 0.4f + 0.6f * pulse);
    }
    for (int i = 0; i < Config::TOP_LED_COUNT; i++) {
        leds.setTopColor((uint8_t)(amberR * 0.75f), (uint8_t)(amberG * 0.75f), 0);
    }
}

// -------------------------------------------------------------
// FEATURE 5: Rick Quotes Soundboard Browser ('R' Dimension)
// -------------------------------------------------------------
void PortalGun::handleRickQuote() {
    const auto& joy = input.getState();
    uint32_t now = millis();
    static uint32_t lastSampleTime = 0;
    static bool wasDirPressed = false;

    if (stateInit) {
        stateInit = false;
        lastSampleTime = now;
        wasDirPressed = false;
        quoteAudioStarted = false;
        
        // Opening portal blast + pick initial quote (dimNumber selects quote index if 1..44)
        if (dimNumber >= 1 && dimNumber <= (int)rick_soundboard_count) {
            currentQuoteIdx = dimNumber - 1;
        } else {
            currentQuoteIdx = (rick_soundboard_count > 0) ? random(rick_soundboard_count) : 0;
        }
        audio.playRaw(snd_portal_open1, snd_portal_open1_size);
    }

    // Wait for the short portal opening blast before speaking
    if (!quoteAudioStarted) {
        if (!audio.isPlaying() || (now - stateStartTime >= 350)) {
            quoteAudioStarted = true;
            if (rick_soundboard_count > 0) {
                audio.playRaw(rick_soundboard[currentQuoteIdx].pcmData, rick_soundboard[currentQuoteIdx].size);
                display.showText(rick_soundboard[currentQuoteIdx].label);
            }
        }
    }

    // Pushing the joystick UP, DOWN, LEFT, or RIGHT samples another quote randomly!
    bool isDirPressed = (joy.up || joy.down || joy.left || joy.right);
    if (isDirPressed && (!wasDirPressed || (now - lastSampleTime > 320))) {
        lastSampleTime = now;
        if (rick_soundboard_count > 0) {
            int nextQuote = random(rick_soundboard_count);
            if (rick_soundboard_count > 1 && nextQuote == currentQuoteIdx) {
                nextQuote = (nextQuote + 1) % rick_soundboard_count;
            }
            currentQuoteIdx = nextQuote;
            audio.stop();
            audio.playRaw(rick_soundboard[currentQuoteIdx].pcmData, rick_soundboard[currentQuoteIdx].size);
            display.showText(rick_soundboard[currentQuoteIdx].label);
        }
    }
    wasDirPressed = isDirPressed;

    // Display subtitle code
    if (quoteAudioStarted && rick_soundboard_count > 0) {
        display.showText(rick_soundboard[currentQuoteIdx].label);
    }

    // Visuals: Animated talking portal
    if (audio.isPlaying()) {
        float pulse = (sinf(now / 110.0f) + 1.0f) * 0.5f;
        leds.setPortalBrightness(0.25f + 0.12f * pulse);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 0, 255, 0);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) {
            leds.setGreen(i, 0.4f + 0.6f * pulse);
        }
    } else {
        // Idle open portal waiting for next quote sample
        float pulse = (sinf(now / 200.0f) + 1.0f) * 0.5f;
        leds.setPortalBrightness(0.20f + 0.05f * pulse);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 0, 220, 20);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) {
            leds.setGreen(i, 0.3f + 0.3f * pulse);
        }
    }
}

enum class SnakePhase { ShowLives, Playing, LostLife, Dead };
static SnakePhase snakePhase = SnakePhase::ShowLives;
static uint32_t snakePhaseStart = 0;
static int snakeLives = 3;

struct SnakeSegment {
    int8_t digit; // 0..3
    int8_t seg;   // 0..6 (a=0, b=1, c=2, d=3, e=4, f=5, g=6)
};

static SnakeSegment snakeBody[28];
static int snakeLen = 3;
static SnakeSegment snakeFood = {1, 6};
static int8_t snakeTargetJunc = 1; // 0:TL, 1:TR, 2:ML, 3:MR, 4:BL, 5:BR
static int8_t snakeMomentum = 3;   // 0:UP, 1:DOWN, 2:LEFT, 3:RIGHT
static uint32_t lastSnakeMove = 0;
static uint32_t snakeSpeed = 220;
static uint32_t snakeDeadTime = 0;
static int snakeScore = 0;

struct SnakeRoute {
    int8_t d_offset;
    int8_t seg;
    int8_t target_junction;
    int8_t new_momentum;
};

// Routing table: [current_target_junction (0..5)][requested_direction (0:UP, 1:DOWN, 2:LEFT, 3:RIGHT)]
static const SnakeRoute SNAKE_ROUTES[6][4] = {
    // Junction 0: TL
    { { 0, 4, 2, 0 }, { 0, 5, 2, 1 }, { -1, 0, 0, 2 }, { 0, 0, 1, 3 } },
    // Junction 1: TR
    { { 0, 2, 3, 0 }, { 0, 1, 3, 1 }, { 0, 0, 0, 2 }, { 1, 0, 1, 3 } },
    // Junction 2: ML
    { { 0, 5, 0, 0 }, { 0, 4, 4, 1 }, { -1, 6, 2, 2 }, { 0, 6, 3, 3 } },
    // Junction 3: MR
    { { 0, 1, 1, 0 }, { 0, 2, 5, 1 }, { 0, 6, 2, 2 }, { 1, 6, 3, 3 } },
    // Junction 4: BL
    { { 0, 4, 2, 0 }, { 0, 5, 0, 1 }, { -1, 3, 4, 2 }, { 0, 3, 5, 3 } },
    // Junction 5: BR
    { { 0, 2, 3, 0 }, { 0, 1, 1, 1 }, { 0, 3, 4, 2 }, { 1, 3, 5, 3 } }
};

static void spawnSnakeFood() {
    bool collides;
    int attempts = 0;
    do {
        collides = false;
        snakeFood.digit = random(4);
        snakeFood.seg = random(7);
        for (int i = 0; i < snakeLen; i++) {
            if (snakeBody[i].digit == snakeFood.digit && snakeBody[i].seg == snakeFood.seg) {
                collides = true;
                break;
            }
        }
        attempts++;
    } while (collides && attempts < 50);
}

void PortalGun::handleSnakeGame() {
    const auto& joy = input.getState();
    uint32_t now = millis();
    
    if (stateInit) {
        stateInit = false;
        snakeLives = (dimNumber > 0) ? min(dimNumber, 99) : 3;
        snakeScore = 0;
        snakePhase = SnakePhase::ShowLives;
        snakePhaseStart = now;
        
        snakeLen = 3;
        snakeMomentum = 3; // RIGHT
        snakeTargetJunc = 1; // TR
        snakeBody[0] = {2, 0}; // Head at Digit 2 a
        snakeBody[1] = {1, 0}; // Neck at Digit 1 a
        snakeBody[2] = {0, 0}; // Tail at Digit 0 a
        snakeSpeed = 220;
        lastSnakeMove = now;
        spawnSnakeFood();
        audio.playRaw(snd_snake_jazz, snd_snake_jazz_size, true); // Loop Snake Jazz!
    }
    
    // Phase 1: ShowLives ("LV 3" or "L020")
    if (snakePhase == SnakePhase::ShowLives) {
        char livesBuf[5];
        if (snakeLives <= 9) {
            snprintf(livesBuf, sizeof(livesBuf), "LV %d", snakeLives);
        } else {
            snprintf(livesBuf, sizeof(livesBuf), "L%03d", snakeLives);
        }
        display.showText(livesBuf);

        // Front 3 green LEDs display current life count HUD
        for (int i = 0; i < 3; i++) {
            leds.setGreen(i, (i < snakeLives) ? 1.0f : 0.0f);
        }

        // Ambient green pulse
        float pulse = (sinf(now / 150.0f) + 1.0f) * 0.5f;
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 0, (uint8_t)(80 + 120 * pulse), 0);
        }
        leds.showPortal();

        if (now - snakePhaseStart >= 1100) {
            snakePhase = SnakePhase::Playing;
            lastSnakeMove = now;
            if (!audio.isPlaying()) {
                audio.playRaw(snd_snake_jazz, snd_snake_jazz_size, true);
            }
        }
        return;
    }

    // Phase 2: LostLife (flashes red, shows life loss)
    if (snakePhase == SnakePhase::LostLife) {
        uint32_t elapsed = now - snakePhaseStart;
        bool flash = ((now / 80) % 2 == 0);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, flash ? 255 : 20, 0, 0);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, flash ? 1.0f : 0.0f);

        char livesBuf[5];
        if (snakeLives <= 9) {
            snprintf(livesBuf, sizeof(livesBuf), "LV %d", snakeLives);
        } else {
            snprintf(livesBuf, sizeof(livesBuf), "L%03d", snakeLives);
        }
        display.showText(livesBuf);

        if (elapsed >= 900) {
            if (snakeLives > 0) {
                // Respawn snake at starting position, preserve score
                snakeLen = 3;
                snakeMomentum = 3; // RIGHT
                snakeTargetJunc = 1; // TR
                snakeBody[0] = {2, 0};
                snakeBody[1] = {1, 0};
                snakeBody[2] = {0, 0};
                spawnSnakeFood();
                snakePhase = SnakePhase::ShowLives;
                snakePhaseStart = now;
                audio.playRaw(snd_snake_jazz, snd_snake_jazz_size, true);
            } else {
                // Out of lives -> LV 0 then Dead
                snakePhase = SnakePhase::Dead;
                snakeDeadTime = now;
                audio.stop();
                audio.playRaw(snd_roulette_boom, snd_roulette_boom_size);
            }
        }
        return;
    }

    // Phase 3: Dead (Game Over)
    if (snakePhase == SnakePhase::Dead) {
        uint32_t deadElapsed = now - snakeDeadTime;
        
        bool flash = ((now / 120) % 2 == 0);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, flash ? 255 : 30, 0, 0);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, flash ? 1.0f : 0.0f);
        
        if (deadElapsed < 1800) {
            display.showText("dEAd");
        } else {
            char scoreBuf[5];
            snprintf(scoreBuf, sizeof(scoreBuf), "SC%02d", snakeScore % 100);
            display.showText(scoreBuf);
        }
        
        // Directly exits after game-over sequence completes (no click needed)
        if (deadElapsed >= 3600) {
            input.clearClicks();
            audio.stop();
            leds.clearPortal();
            for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
            changeState(PortalState::DimensionSelection);
            return;
        }
        return;
    }

    // Phase 4: Active Gameplay
    // Joystick direction request
    int requestedDir = -1;
    if (joy.up) requestedDir = 0;
    else if (joy.down) requestedDir = 1;
    else if (joy.left) requestedDir = 2;
    else if (joy.right) requestedDir = 3;
    
    // Prevent immediate 180-degree self reversal into own neck
    if (snakeLen > 1) {
        if (requestedDir == 0 && snakeMomentum == 1) requestedDir = -1;
        else if (requestedDir == 1 && snakeMomentum == 0) requestedDir = -1;
        else if (requestedDir == 2 && snakeMomentum == 3) requestedDir = -1;
        else if (requestedDir == 3 && snakeMomentum == 2) requestedDir = -1;
    }
    
    // Snake movement timer
    if (now - lastSnakeMove >= snakeSpeed) {
        lastSnakeMove = now;
        
        int effDir = snakeMomentum;
        if (requestedDir != -1) {
            effDir = requestedDir;
        } else {
            // Natural corner momentum when no steering is actively pressed
            if (snakeTargetJunc == 5 && snakeMomentum == 1) effDir = 2; // BR moving down -> turns left
            else if (snakeTargetJunc == 4 && snakeMomentum == 1) effDir = 3; // BL moving down -> turns right
            else if (snakeTargetJunc == 1 && snakeMomentum == 0) effDir = 2; // TR moving up -> turns left
            else if (snakeTargetJunc == 0 && snakeMomentum == 0) effDir = 3; // TL moving up -> turns right
        }
        
        const SnakeRoute& route = SNAKE_ROUTES[snakeTargetJunc][effDir];
        int nextD = (snakeBody[0].digit + route.d_offset + 4) % 4;
        int nextS = route.seg;
        int nextTJ = route.target_junction;
        int newMom = route.new_momentum;
        
        // Check self collision (head colliding with body)
        for (int i = 0; i < snakeLen - 1; i++) {
            if (snakeBody[i].digit == nextD && snakeBody[i].seg == nextS) {
                snakeLives--;
                audio.stop();
                audio.playRaw(snd_roulette_clack, snd_roulette_clack_size);
                snakePhase = SnakePhase::LostLife;
                snakePhaseStart = now;
                return;
            }
        }
        
        // Check if eating food
        bool ate = (nextD == snakeFood.digit && nextS == snakeFood.seg);
        if (ate) {
            snakeScore++;
            if (snakeLen < 27) snakeLen++;
            audio.playRaw(snd_roulette_clack, snd_roulette_clack_size);
            spawnSnakeFood();
            if (snakeSpeed > 130) snakeSpeed -= 5;
        }
        
        // Shift body forward
        for (int i = snakeLen - 1; i > 0; i--) {
            snakeBody[i] = snakeBody[i - 1];
        }
        
        snakeBody[0] = { (int8_t)nextD, (int8_t)nextS };
        snakeTargetJunc = nextTJ;
        snakeMomentum = newMom;
    }
    
    // Render snake display
    uint8_t dispData[4] = {0, 0, 0, 0};
    for (int i = 0; i < snakeLen; i++) {
        if (snakeBody[i].digit >= 0 && snakeBody[i].digit < 4) {
            dispData[snakeBody[i].digit] |= (1 << snakeBody[i].seg);
        }
    }
    
    // Blinking food segment (at ~4 Hz)
    if ((now / 150) % 2 == 0) {
        if (snakeFood.digit >= 0 && snakeFood.digit < 4) {
            dispData[snakeFood.digit] |= (1 << snakeFood.seg);
        }
    }
    display.showSegments(dispData);
    
    // Front 3 LEDs show live life count HUD during gameplay
    for (int i = 0; i < 3; i++) {
        leds.setGreen(i, (i < snakeLives) ? 0.9f : 0.0f);
    }

    // Dynamic green ambient lighting following the snake
    int activeDigit = snakeBody[0].digit;
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        leds.setPortalColor(i, 0, (i == activeDigit + 1) ? 220 : 35, 0);
    }
    leds.showPortal();
}

// -------------------------------------------------------------
// FEATURE 3: Interdimensional Chamber Roulette ('G' Dimension)
// -------------------------------------------------------------
enum class RoulettePhase { Spin, Ready, SafeResult, BoomBlast, BoomQuote };
static RoulettePhase roulettePhase = RoulettePhase::Spin;
static uint32_t roulettePhaseStart = 0;
static int rouletteTotalChambers = 6;
static int rouletteBulletSlot = 0;   // 0..rouletteTotalChambers-1
static int rouletteCurrentSlot = 0;  // Current trial (0..rouletteTotalChambers-1)

void PortalGun::handleRoulette() {
    const auto& joy = input.getState();
    uint32_t now = millis();
    uint32_t elapsed = now - roulettePhaseStart;
    
    if (stateInit) {
        stateInit = false;
        roulettePhase = RoulettePhase::Spin;
        roulettePhaseStart = now;
        // Bullet randomly hidden in 1 of the chambers based on dimNumber (default 6)
        rouletteTotalChambers = (dimNumber > 1) ? min(dimNumber, 99) : 6;
        rouletteBulletSlot = random(rouletteTotalChambers);
        rouletteCurrentSlot = 0;
        audio.playRaw(snd_roulette_spin, snd_roulette_spin_size);
    }
    
    if (roulettePhase == RoulettePhase::Spin) {
        display.showText("SPIN");
        
        // Rotating mechanical cylinder LED animation decelerating with the ratchet sound
        int spinSlot = (elapsed / 90) % 6;
        for (int i = 1; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 0, (i == spinSlot + 1) ? 255 : 15, 0);
        }
        leds.setPortalColor(0, 0, 50, 0); // center dim
        leds.showPortal();
        
        for (int i = 0; i < 3; i++) {
            leds.setGreen(i, (i == (spinSlot % 3)) ? 1.0f : 0.1f);
        }
        
        if (!audio.isPlaying() || elapsed >= 1600) {
            roulettePhase = RoulettePhase::Ready;
            roulettePhaseStart = now;
        }
    } else if (roulettePhase == RoulettePhase::Ready) {
        // Display alternates between "PULL" and "CH x" showing trial count
        char chBuf[5];
        if (rouletteTotalChambers <= 9) {
            snprintf(chBuf, sizeof(chBuf), "CH %d", rouletteCurrentSlot + 1);
        } else {
            snprintf(chBuf, sizeof(chBuf), "C%03d", rouletteCurrentSlot + 1);
        }
        display.showText(((now / 900) % 2 == 0) ? "PULL" : chBuf);
        
        // Tense heartbeat pulse on LEDs with live chamber cylinder HUD:
        float pulse = (sinf(now / 150.0f) + 1.0f) * 0.5f;
        leds.setPortalColor(0, (uint8_t)(80.0f * pulse), (uint8_t)(160.0f * (1.0f - pulse)), 0);
        
        if (rouletteTotalChambers <= 6) {
            for (int i = 0; i < 6; i++) {
                if (i < rouletteCurrentSlot) {
                    leds.setPortalColor(i + 1, 0, 90, 20); // Already safely cleared
                } else if (i == rouletteCurrentSlot) {
                    leds.setPortalColor(i + 1, (uint8_t)(255.0f * pulse), (uint8_t)(160.0f * (1.0f - pulse)), 0);
                } else if (i < rouletteTotalChambers) {
                    leds.setPortalColor(i + 1, 40, 25, 0); // Unfired
                } else {
                    leds.setPortalColor(i + 1, 0, 0, 0); // Unused cylinder slot
                }
            }
        } else {
            int activeLed = rouletteCurrentSlot % 6;
            for (int i = 0; i < 6; i++) {
                if (i == activeLed) {
                    leds.setPortalColor(i + 1, (uint8_t)(255.0f * pulse), (uint8_t)(160.0f * (1.0f - pulse)), 0);
                } else if (i < activeLed) {
                    leds.setPortalColor(i + 1, 0, 90, 20);
                } else {
                    leds.setPortalColor(i + 1, 40, 25, 0);
                }
            }
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) {
            leds.setGreen(i, 0.2f + 0.5f * pulse);
        }
        
        // Pull trigger: user clicks joystick
        if (joy.clicked) {
            roulettePhaseStart = now;
            if (rouletteCurrentSlot != rouletteBulletSlot) {
                // Empty chamber dry fire: metallic clack
                roulettePhase = RoulettePhase::SafeResult;
                audio.playRaw(snd_roulette_clack, snd_roulette_clack_size);
                display.showText("SAFE");
            } else {
                // Loaded live chamber: portal explosive blast!
                roulettePhase = RoulettePhase::BoomBlast;
                audio.playRaw(snd_roulette_boom, snd_roulette_boom_size);
                display.showText("bOOM");
            }
        }
    } else if (roulettePhase == RoulettePhase::SafeResult) {
        display.showText("SAFE");
        
        // Crisp cyan-green flash
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 0, 220, 100);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, 0.8f);
        
        if (elapsed > 1400 || (elapsed > 350 && joy.clicked)) {
            // Advance to the next chamber!
            rouletteCurrentSlot++;
            if (rouletteCurrentSlot >= rouletteTotalChambers) {
                // Survived all trials! Disarm and return
                leds.clearPortal();
                for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
                changeState(PortalState::DimensionSelection);
            } else {
                roulettePhase = RoulettePhase::Ready;
                roulettePhaseStart = now;
            }
        }
    } else if (roulettePhase == RoulettePhase::BoomBlast) {
        display.showText("bOOM");
        
        // Intense red strobe blast!
        bool strobe = ((now / 50) % 2 == 0);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, strobe ? 255 : 20, 0, 0);
        }
        leds.showPortal();
        for (int i = 0; i < Config::TOP_LED_COUNT; i++) {
            leds.setTopColor(strobe ? 255 : 0, 0, 0);
        }
        for (int i = 0; i < 3; i++) {
            leds.setGreen(i, strobe ? 1.0f : 0.0f);
        }
        
        // Once boom finishes, Rick gloats: "Riggity Riggity Wrecked Son!"
        if (!audio.isPlaying() || elapsed > 1800) {
            roulettePhase = RoulettePhase::BoomQuote;
            roulettePhaseStart = now;
            audio.playRaw(snd_roulette_wrecked, snd_roulette_wrecked_size);
            display.showText("WREC");
        }
    } else if (roulettePhase == RoulettePhase::BoomQuote) {
        display.showText("WREC");
        
        float pulse = (sinf(now / 180.0f) + 1.0f) * 0.5f;
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, (uint8_t)(255.0f * pulse), (uint8_t)(100.0f * (1.0f - pulse)), 0);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) {
            leds.setGreen(i, 0.5f + 0.5f * pulse);
        }
        
        if (!audio.isPlaying()) {
            input.clearClicks();
            audio.stop();
            leds.clearPortal();
            for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
            changeState(PortalState::DimensionSelection);
        }
    }
}

// -------------------------------------------------------------
// FEATURE 8: Google Chrome Dinosaur Runner ('D' Dimension)
// -------------------------------------------------------------
enum class DinoJumpState { Ground, Rise, Apex, Fall };
static DinoJumpState dinoJump = DinoJumpState::Ground;
static uint32_t dinoJumpStart = 0;
static const uint32_t DINO_JUMP_TOTAL_MS = 620; // Specific air time
static const uint32_t DINO_JUMP_RISE_MS  = 160;
static const uint32_t DINO_JUMP_APEX_MS  = 460;

// Track on 4 digits: 0 is player, 1-3 are incoming obstacles
// 0: Clear, 1: Small Cactus, 2: Tall Cactus, 3: Low Bird, 4: High Bird
static uint8_t dinoTrack[4] = {0, 0, 0, 0};
static uint32_t lastDinoStep = 0;
static uint32_t dinoStepInterval = 330;
static int dinoEmptyCount = 2;

enum class DinoPhase { ShowLives, Playing, LostLife, Dead };
static DinoPhase dinoPhase = DinoPhase::ShowLives;
static uint32_t dinoPhaseStart = 0;
static int dinoLives = 3;
static uint32_t dinoDeadTime = 0;
static int dinoAliveSeconds = 0;
static uint32_t dinoTotalPlayMs = 0;
static uint32_t lastDinoPlayTick = 0;

static uint8_t getDinoObstacleBits(uint8_t obsType) {
    switch (obsType) {
        case 1: return 0x08;        // Small Cactus: bottom 'd'
        case 2: return 0x08 | 0x40; // Tall Cactus: bottom 'd' + middle 'g'
        case 3: return 0x40;        // Low Bird: middle 'g'
        case 4: return 0x01;        // High Bird: top 'a'
        default: return 0x00;
    }
}

void PortalGun::handleDinoGame() {
    const auto& joy = input.getState();
    uint32_t now = millis();
    
    if (stateInit) {
        stateInit = false;
        dinoLives = (dimNumber > 0) ? min(dimNumber, 99) : 3;
        dinoPhase = DinoPhase::ShowLives;
        dinoPhaseStart = now;
        dinoJump = DinoJumpState::Ground;
        dinoJumpStart = 0;
        for (int i = 0; i < 4; i++) dinoTrack[i] = 0;
        lastDinoStep = now;
        dinoStepInterval = 330;
        dinoDeadTime = 0;
        dinoAliveSeconds = 0;
        dinoTotalPlayMs = 0;
        lastDinoPlayTick = now;
        dinoEmptyCount = 2;
        audio.playRaw(snd_portal_open1, snd_portal_open1_size);
    }
    
    // Phase 1: ShowLives ("LV 3" or "L020")
    if (dinoPhase == DinoPhase::ShowLives) {
        char livesBuf[5];
        if (dinoLives <= 9) {
            snprintf(livesBuf, sizeof(livesBuf), "LV %d", dinoLives);
        } else {
            snprintf(livesBuf, sizeof(livesBuf), "L%03d", dinoLives);
        }
        display.showText(livesBuf);

        // Front 3 green LEDs display current life count HUD
        for (int i = 0; i < 3; i++) {
            leds.setGreen(i, (i < dinoLives) ? 1.0f : 0.0f);
        }

        // Ambient green pulse
        float pulse = (sinf(now / 150.0f) + 1.0f) * 0.5f;
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 0, (uint8_t)(80 + 120 * pulse), 0);
        }
        leds.showPortal();

        if (now - dinoPhaseStart >= 1100) {
            dinoPhase = DinoPhase::Playing;
            lastDinoStep = now;
            lastDinoPlayTick = now;
        }
        return;
    }

    // Phase 2: LostLife (flashes red, shows life loss)
    if (dinoPhase == DinoPhase::LostLife) {
        uint32_t elapsed = now - dinoPhaseStart;
        bool flash = ((now / 80) % 2 == 0);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, flash ? 255 : 20, 0, 0);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, flash ? 1.0f : 0.0f);

        char livesBuf[5];
        if (dinoLives <= 9) {
            snprintf(livesBuf, sizeof(livesBuf), "LV %d", dinoLives);
        } else {
            snprintf(livesBuf, sizeof(livesBuf), "L%03d", dinoLives);
        }
        display.showText(livesBuf);

        if (elapsed >= 900) {
            if (dinoLives > 0) {
                // Clear immediate obstacle track and reset jump so dino doesn't land on obstacle
                for (int i = 0; i < 4; i++) dinoTrack[i] = 0;
                dinoJump = DinoJumpState::Ground;
                dinoEmptyCount = 2;
                dinoPhase = DinoPhase::ShowLives;
                dinoPhaseStart = now;
            } else {
                // Out of lives -> LV 0 then Dead
                dinoPhase = DinoPhase::Dead;
                dinoDeadTime = now;
                audio.stop();
                audio.playRaw(snd_roulette_boom, snd_roulette_boom_size);
            }
        }
        return;
    }

    // Phase 3: Dead (Game Over)
    if (dinoPhase == DinoPhase::Dead) {
        uint32_t deadElapsed = now - dinoDeadTime;
        
        bool flash = ((now / 120) % 2 == 0);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, flash ? 255 : 30, 0, 0);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, flash ? 1.0f : 0.0f);
        
        if (deadElapsed < 1800) {
            display.showText("dEAd");
        } else {
            // Display your score: alive seconds!
            char scoreBuf[5];
            snprintf(scoreBuf, sizeof(scoreBuf), "%3ds", dinoAliveSeconds);
            display.showText(scoreBuf);
        }
        
        // Directly exits after game-over sequence completes (no click needed)
        if (deadElapsed >= 3600) {
            input.clearClicks();
            audio.stop();
            leds.clearPortal();
            for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
            changeState(PortalState::DimensionSelection);
            return;
        }
        return;
    }

    // Phase 4: Active Gameplay
    // Alive time update
    dinoTotalPlayMs += (now - lastDinoPlayTick);
    lastDinoPlayTick = now;
    dinoAliveSeconds = dinoTotalPlayMs / 1000;
    
    // Dynamic difficulty: speed up as seconds pass (from 330ms down to 160ms)
    uint32_t speedUp = min((uint32_t)170, (uint32_t)((dinoAliveSeconds / 4) * 15));
    dinoStepInterval = 330 - speedUp;
    
    // Jump trigger (UP only)
    if (joy.up && dinoJump == DinoJumpState::Ground) {
        dinoJump = DinoJumpState::Rise;
        dinoJumpStart = now;
        audio.playRaw(snd_roulette_clack, snd_roulette_clack_size); // Crisp jump sound
    }
    
    // Jump physics & air time state
    if (dinoJump != DinoJumpState::Ground) {
        uint32_t jumpElapsed = now - dinoJumpStart;
        if (jumpElapsed < DINO_JUMP_RISE_MS) {
            dinoJump = DinoJumpState::Rise;
        } else if (jumpElapsed < DINO_JUMP_APEX_MS) {
            dinoJump = DinoJumpState::Apex;
        } else if (jumpElapsed < DINO_JUMP_TOTAL_MS) {
            dinoJump = DinoJumpState::Fall;
        } else {
            dinoJump = DinoJumpState::Ground;
        }
    }
    
    // Track advancement (obstacles slide from Digit 3 to Digit 0)
    if (now - lastDinoStep >= dinoStepInterval) {
        lastDinoStep = now;
        
        dinoTrack[0] = dinoTrack[1];
        dinoTrack[1] = dinoTrack[2];
        dinoTrack[2] = dinoTrack[3];
        
        // Spawn logic on Digit 3
        if (dinoEmptyCount < 2) {
            // Keep mandatory jump clearance gap
            dinoTrack[3] = 0;
            dinoEmptyCount++;
        } else {
            int spawnChance = 45 + min(35, (int)(dinoAliveSeconds * 1.5f));
            if (random(100) < spawnChance) {
                int roll = random(100);
                if (dinoAliveSeconds < 8) {
                    dinoTrack[3] = 1; // Small Cactus
                } else if (dinoAliveSeconds < 20) {
                    dinoTrack[3] = (roll < 60) ? 1 : 2; // Small or Tall Cactus
                } else {
                    if (roll < 35) dinoTrack[3] = 1;      // Small Cactus
                    else if (roll < 60) dinoTrack[3] = 2; // Tall Cactus
                    else if (roll < 80) dinoTrack[3] = 3; // Low Flying Bird
                    else dinoTrack[3] = 4;                // High Flying Bird
                }
                dinoEmptyCount = 0;
            } else {
                dinoTrack[3] = 0;
                dinoEmptyCount++;
            }
        }
    }
    
    // Determine player's active segments
    uint8_t dinoBits = 0;
    if (dinoJump == DinoJumpState::Ground) {
        if (joy.down) {
            // Ducking stance (flat on ground segment 'd')
            dinoBits = 0x08;
        } else {
            // Running leg animation (alternating e=0x10 and c=0x04)
            uint8_t leg = ((now / 120) % 2 == 0) ? 0x10 : 0x04;
            dinoBits = 0x08 | leg;
        }
    } else if (dinoJump == DinoJumpState::Rise || dinoJump == DinoJumpState::Fall) {
        // Mid-air (waist height 'g')
        dinoBits = 0x40;
    } else if (dinoJump == DinoJumpState::Apex) {
        // High peak apex (top height 'a')
        dinoBits = 0x01;
    }
    
    // Collision detection with obstacle at Digit 0
    if (dinoTrack[0] != 0) {
        uint8_t obsBits = getDinoObstacleBits(dinoTrack[0]);
        if ((dinoBits & obsBits) != 0) {
            // Hit obstacle! Lose life
            dinoLives--;
            audio.stop();
            audio.playRaw(snd_roulette_clack, snd_roulette_clack_size);
            dinoPhase = DinoPhase::LostLife;
            dinoPhaseStart = now;
            return;
        }
    }
    
    // Render 7-segment display
    uint8_t dispData[4] = {0, 0, 0, 0};
    uint8_t currObsBits = getDinoObstacleBits(dinoTrack[0]);
    dispData[0] = dinoBits | currObsBits;
    dispData[1] = getDinoObstacleBits(dinoTrack[1]);
    dispData[2] = getDinoObstacleBits(dinoTrack[2]);
    dispData[3] = getDinoObstacleBits(dinoTrack[3]);
    display.showSegments(dispData);
    
    // Front 3 LEDs show live life count HUD during gameplay (same as Snake)
    for (int i = 0; i < 3; i++) {
        leds.setGreen(i, (i < dinoLives) ? 0.9f : 0.0f);
    }
    
    // Portal ring: ambient runner pulse, jumps up when player jumps
    if (dinoJump != DinoJumpState::Ground) {
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 0, 255, 120);
        }
    } else {
        float pulse = (sinf(now / 160.0f) + 1.0f) * 0.5f;
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, 0, (uint8_t)(60 + 60 * pulse), 0);
        }
    }
    leds.showPortal();
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
