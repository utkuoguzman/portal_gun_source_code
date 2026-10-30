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
#include <driver/gpio.h>
#include <driver/rtc_io.h>

PortalGun portalGun;

static char dimLetter = 'C';
static int dimNumber = 137;
static unsigned long portalOpenTime = 0;
static bool portalIsOpen = false;
static bool portalJustClosed = false;
static float frontLedSmooth[3] = {0.0f, 0.0f, 0.0f};

static void printDimensionInfo() {
    const char* modeHint = "";
    switch (dimLetter) {
        case 'C': modeHint = (dimNumber == 137) ? " [Earth Dimension C-137 (Rick's Home)]" : " [Dimension C Series]"; break;
        case 'B': modeHint = " [Citadel Radar - Double Click to Activate]"; break;
        case 'U': modeHint = " [Wi-Fi Portal & Web OTA - Double Click to Activate]"; break;
        case 'P': modeHint = " [Party Mode (Get Schwifty!) - Double Click to Activate]"; break;
        case 'E': modeHint = " [Evil Morty Amber Portal - Double Click to Activate]"; break;
        case 'S': modeHint = " [Snake Game - Double Click to Play]"; break;
        case 'R': 
        case 'G': modeHint = " [Chamber Roulette - Double Click to Play]"; break;
        case 'D': modeHint = " [Dino Runner Game - Double Click to Play]"; break;
        case 'Q': modeHint = " [Rick Quotes Soundboard - Double Click to Browse]"; break;
        case 'L': modeHint = " [Flashlight / Torch - Double Click to Activate]"; break;
        default:  modeHint = " [Multiverse Coordinates - Single Click to Fire]"; break;
    }
    Serial.printf("[Dimension Dial] -> %c-%03d%s\n", dimLetter, dimNumber, modeHint);
}

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
    
    // Global Power Button override or 3-second Joystick button hold (if not already powering down/sleeping)
    if ((joy.powerJustPressed || joy.justHeld3s) && currentState != PortalState::PowerDown && currentState != PortalState::DeepSleep) {
        if (currentState == PortalState::WifiPortal) wifiPortal.stopPortal();
        if (currentState == PortalState::RadarScan) radar.stopScan();
        changeState(PortalState::PowerDown);
        return;
    }

    // Universal 3-click escape: Return to Dimension Selection from ANY mode, game, torch, party, or firing portal!
    if (joy.tripleClicked) {
        Serial.println("[Input] Triple Click (Escape) -> Returning to Dimension Selection");
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
    
    Serial.print("\n>>> [State] -> ");
    switch (currentState) {
        case PortalState::PowerUp: 
            Serial.println("PowerUp (Initial Boot Animation)"); 
            break;
        case PortalState::PowerUpFinalize: 
            Serial.println("PowerUpFinalize (Buckle Up Rick Intro)"); 
            break;
        case PortalState::DimensionSelection: 
            Serial.printf("DimensionSelection (Current: %c-%03d)\n", dimLetter, dimNumber); 
            break;
        case PortalState::Party: 
            Serial.println("Party Mode (Get Schwifty!)"); 
            break;
        case PortalState::Torch: 
            Serial.println("Torch / Flashlight Mode"); 
            break;
        case PortalState::RadarScan: 
            Serial.println("RadarScan (Citadel / Federation Bluetooth Proximity Scanner)"); 
            break;
        case PortalState::WifiPortal: 
            Serial.println("WifiPortal (Web AP: 'PortalGun-AP' | IP: 192.168.4.1 | OTA: http://192.168.4.1/update)"); 
            break;
        case PortalState::EvilMorty: 
            Serial.println("EvilMorty (Amber Golden Portal - For The Damaged Coda)"); 
            break;
        case PortalState::SnakeGame: 
            Serial.println("SnakeGame (7-Segment Snake)"); 
            break;
        case PortalState::Roulette: 
            Serial.println("Roulette (Interdimensional Chamber Roulette)"); 
            break;
        case PortalState::DinoGame: 
            Serial.println("DinoGame (Google Chrome T-Rex Runner)"); 
            break;
        case PortalState::RickQuote: 
            Serial.println("RickQuote (Interdimensional Rick Soundboard Browser)"); 
            break;
        case PortalState::PowerDown: 
            Serial.println("PowerDown (Powering Down)"); 
            break;
        case PortalState::DeepSleep: 
            Serial.println("DeepSleep (Entering Ultra-Low Power Standby)"); 
            break;
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
        
        // Calculate scroll speed (2x slower than audio duration for comfortable reading)
        // Note: snd_buckleup_size is 4-bit ADPCM (2 samples per byte at 8400 Hz)
        uint32_t audioLengthMs = (snd_buckleup_size > 0) ? (snd_buckleup_size * 2 * 1000) / 8400 : 3000;
        if (audioLengthMs == 0) audioLengthMs = 3000;
        
        // "BUCKLE UP BITCHES " is 18 chars. Total steps = 18 + 4 = 22.
        // Multiply step interval by 2 for 2x slower scrolling (~126 ms/step)
        int speed = (audioLengthMs * 2) / 22; 
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
        printDimensionInfo();
    }
    
    if (portalIsOpen) {
        // Portal beam open: closes automatically after 5000ms timeout (or early via 3 clicks in update())
        if (millis() - portalOpenTime >= 5000) {
            portalIsOpen = false;
            portalJustClosed = true;
            Serial.println("[Portal Gun] Portal beam closed.");
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
            Serial.printf("[Input] Double Click -> Activating Dimension %c-%03d!\n", dimLetter, dimNumber);
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
            input.clearClicks();
            portalIsOpen = true;
            portalOpenTime = millis();
            Serial.printf("[Input] Single Click -> Opening Portal Beam to %c-%03d!\n", dimLetter, dimNumber);
            if (millis() % 2 == 0) {
                audio.playRaw(snd_portal_open1, snd_portal_open1_size);
            } else {
                audio.playRaw(snd_portal_open2, snd_portal_open2_size);
            }
        }
        
        static unsigned long lastMove = 0;
        bool dimChanged = false;
        if (millis() - lastMove > 200) {
            if (joy.right) {
                dimLetter++;
                if (dimLetter > 'Z') dimLetter = 'A';
                if (dimLetter == 'S' || dimLetter == 'D') dimNumber = 5;
                lastMove = millis();
                dimChanged = true;
            } else if (joy.left) {
                dimLetter--;
                if (dimLetter < 'A') dimLetter = 'Z';
                if (dimLetter == 'S' || dimLetter == 'D') dimNumber = 5;
                lastMove = millis();
                dimChanged = true;
            }
            
            if (joy.up) {
                dimNumber++;
                if (dimNumber > 999) dimNumber = 0;
                lastMove = millis();
                dimChanged = true;
            } else if (joy.down) {
                dimNumber--;
                if (dimNumber < 0) dimNumber = 999;
                lastMove = millis();
                dimChanged = true;
            }
        }

        if (dimChanged) {
            printDimensionInfo();
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
    uint32_t now = millis();
    uint32_t elapsed = now - stateStartTime;

    static uint32_t lastTickMs = 0;
    static uint32_t pairedStartMs = 0;
    static float blinkAccumMs = 0.0f;
    static bool blinkOn = true;
    static bool wasPaired = false;

    if (stateInit) {
        stateInit = false;
        lastTickMs = now;
        pairedStartMs = now;
        blinkAccumMs = 0.0f;
        blinkOn = true;
        wasPaired = radar.isPaired();
        radar.startScan();
        audio.playRaw(snd_portal_open1, snd_portal_open1_size);
    }

    // Give 5 full minutes (300s) while waiting to pair, and 3 minutes (180s) once paired (or 3-click to exit anytime)
    uint32_t activeElapsed = wasPaired ? (now - pairedStartMs) : elapsed;
    uint32_t maxDurationMs = wasPaired ? 180000 : 300000;
    if (activeElapsed >= maxDurationMs || joy.tripleClicked) {
        input.clearClicks();
        radar.stopScan();
        audio.playRaw(snd_portal_close1, snd_portal_close1_size);
        leds.clearPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, 0.0f);
        changeState(PortalState::DimensionSelection);
        return;
    }

    uint32_t dt = now - lastTickMs;
    lastTickMs = now;

    // Wait for user to pair "Portal-Gun-C137" on their phone (or single-click joystick if already paired on phone)
    if (!wasPaired) {
        if (radar.isPaired() || joy.singleClicked) {
            wasPaired = true;
            pairedStartMs = now;
            input.clearClicks();
            audio.playRaw(snd_portal_open2, snd_portal_open2_size);
        } else {
            display.showText("PAIR");
            int step = (elapsed / 140) % 6;
            leds.setRadarSweep(step, false);
            return;
        }
    }

    // Paired & sensing distance:
    // prox = 0.0f (far away -> Green, slow "R1CK" blink)
    // prox = 1.0f (very close -> Red, rapid "R1CK" blink)
    float prox = radar.getProximity();

    // 1. Screen turn-on / turn-off animation of "R1CK" gets progressively faster with proximity
    // Half-period: 650ms (at prox=0.0) down to 65ms (at prox=1.0)
    float halfPeriodMs = 650.0f - (prox * 585.0f);
    if (halfPeriodMs < 65.0f) halfPeriodMs = 65.0f;

    blinkAccumMs += (float)dt;
    if (blinkAccumMs >= halfPeriodMs) {
        blinkAccumMs = fmodf(blinkAccumMs, halfPeriodMs);
        blinkOn = !blinkOn;
    }

    if (blinkOn) {
        display.showText("R1CK");
    } else {
        display.clear();
    }

    // 2. Lights turn progressively from Green (far) -> Yellow/Amber (mid) -> Red (close)
    uint8_t r = 0, g = 255;
    if (prox < 0.5f) {
        // 0.0 .. 0.5: Pure Green (0, 255, 0) -> Yellow (255, 255, 0)
        r = (uint8_t)((prox * 2.0f) * 255.0f);
        g = 255;
    } else {
        // 0.5 .. 1.0: Yellow (255, 255, 0) -> Pure Red (255, 0, 0)
        r = 255;
        g = (uint8_t)((1.0f - (prox - 0.5f) * 2.0f) * 255.0f);
    }

    leds.setPortalBrightness(0.25f + 0.20f * prox);
    uint8_t ringR = blinkOn ? r : (uint8_t)(r * 0.25f);
    uint8_t ringG = blinkOn ? g : (uint8_t)(g * 0.25f);
    for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
        leds.setPortalColor(i, ringR, ringG, 0);
    }
    leds.showPortal();

    for (int i = 0; i < Config::TOP_LED_COUNT; i++) {
        leds.setTopColor(ringR, ringG, 0);
    }

    for (int i = 0; i < 3; i++) {
        leds.setGreen(i, blinkOn ? (0.4f + 0.6f * prox) : 0.05f);
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
static uint32_t snakeSpeed = 440;
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
        snakeSpeed = 440;
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

    // Phase 2: LostLife (flashes red, shows current score then transitions to ShowLives / Dead)
    if (snakePhase == SnakePhase::LostLife) {
        uint32_t elapsed = now - snakePhaseStart;
        bool flash = ((now / 80) % 2 == 0);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, flash ? 255 : 20, 0, 0);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, flash ? 1.0f : 0.0f);

        char scoreBuf[5];
        snprintf(scoreBuf, sizeof(scoreBuf), "SC%02d", snakeScore % 100);
        display.showText(scoreBuf);

        if (elapsed >= 1200) {
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
                // Out of lives -> Dead
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
            spawnSnakeFood();
            if (snakeSpeed > 260) snakeSpeed -= 10;
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

// Track on 4 digits: 0 is player, 1-3 are incoming obstacles
// 0: Clear, 1: Ground ('d'), 3: Mid-level ('g'), 4: Upper-level ('a') (No 2-row obstacle)
static uint8_t dinoTrack[4] = {0, 0, 0, 0};
static uint32_t lastDinoStep = 0;
static uint32_t dinoStepInterval = 520;
static int dinoEmptyCount = 3;
static int dinoObstaclesSpawned = 0;
static uint32_t dinoSpeedupStartMs = 0;

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
        case 1: return 0x08;        // Ground obstacle: bottom 'd'
        case 2: return 0x08 | 0x40; // (Unused 2-row obstacle)
        case 3: return 0x40;        // Mid-level obstacle: middle 'g'
        case 4: return 0x01;        // Upper-level obstacle: top 'a'
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
        dinoStepInterval = 520;
        dinoDeadTime = 0;
        dinoAliveSeconds = 0;
        dinoTotalPlayMs = 0;
        lastDinoPlayTick = now;
        dinoEmptyCount = 2;
        dinoObstaclesSpawned = 0;
        dinoSpeedupStartMs = 0;
        audio.playRaw(snd_dino_song, snd_dino_song_size, true); // Loop Dino Song!
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
            if (!audio.isPlaying()) {
                audio.playRaw(snd_dino_song, snd_dino_song_size, true);
            }
        }
        return;
    }

    // Phase 2: LostLife (flashes red, shows score every time you die, then transitions to ShowLives / Dead)
    if (dinoPhase == DinoPhase::LostLife) {
        uint32_t elapsed = now - dinoPhaseStart;
        bool flash = ((now / 80) % 2 == 0);
        for (int i = 0; i < Config::PORTAL_LED_COUNT; i++) {
            leds.setPortalColor(i, flash ? 255 : 20, 0, 0);
        }
        leds.showPortal();
        for (int i = 0; i < 3; i++) leds.setGreen(i, flash ? 1.0f : 0.0f);

        char scoreBuf[5];
        snprintf(scoreBuf, sizeof(scoreBuf), "%3ds", dinoAliveSeconds);
        display.showText(scoreBuf);

        if (elapsed >= 1200) {
            if (dinoLives > 0) {
                // Clear immediate obstacle track and reset jump so dino doesn't land on obstacle
                for (int i = 0; i < 4; i++) dinoTrack[i] = 0;
                dinoJump = DinoJumpState::Ground;
                dinoEmptyCount = 2;
                dinoObstaclesSpawned = 0;
                dinoSpeedupStartMs = 0;
                dinoPhase = DinoPhase::ShowLives;
                dinoPhaseStart = now;
                audio.playRaw(snd_dino_song, snd_dino_song_size, true);
            } else {
                // Out of lives -> Dead
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
    
    // Dynamic difficulty:
    // Steady 520ms shift for the first 2 obstacles, then progressively faster starting on the 3rd obstacle!
    if (dinoObstaclesSpawned < 3) {
        dinoStepInterval = 520;
    } else {
        uint32_t postThirdSec = (dinoTotalPlayMs >= dinoSpeedupStartMs)
                                    ? ((dinoTotalPlayMs - dinoSpeedupStartMs) / 1000)
                                    : 0;
        uint32_t extraObs = (uint32_t)(dinoObstaclesSpawned - 2);
        uint32_t speedUp = min((uint32_t)330, extraObs * 25 + postThirdSec * 10);
        dinoStepInterval = 520 - speedUp; // Progressively accelerates from 520ms down to 190ms
    }
    
    // Jump trigger (UP on joystick)
    if (joy.up && dinoJump == DinoJumpState::Ground) {
        dinoJump = DinoJumpState::Rise;
        dinoJumpStart = now;
    }
    
    // Jump physics & air time state (scaled to current landscape shift speed so jump spans ~2.85 shifts)
    if (dinoJump != DinoJumpState::Ground) {
        uint32_t jumpElapsed = now - dinoJumpStart;
        uint32_t jumpTotalMs = (dinoStepInterval * 285) / 100;
        uint32_t jumpRiseMs  = (dinoStepInterval * 48) / 100;
        uint32_t jumpApexMs  = (dinoStepInterval * 237) / 100;
        if (jumpElapsed < jumpRiseMs) {
            dinoJump = DinoJumpState::Rise;
        } else if (jumpElapsed < jumpApexMs) {
            dinoJump = DinoJumpState::Apex;
        } else if (jumpElapsed < jumpTotalMs) {
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
        
        // Spawn logic on Digit 3 (single-row obstacles: 1=ground 'd', 3=mid 'g', 4=upper 'a'; never 2-row)
        if (dinoEmptyCount < 3) {
            // Keep mandatory clearance gap
            dinoTrack[3] = 0;
            dinoEmptyCount++;
        } else {
            int spawnChance = (dinoObstaclesSpawned < 2) ? 100 : (55 + min(30, (int)(dinoAliveSeconds * 2)));
            if (random(100) < spawnChance) {
                dinoObstaclesSpawned++;
                if (dinoObstaclesSpawned == 1) {
                    // 1st obstacle: always ground ('d')
                    dinoTrack[3] = 1;
                } else if (dinoObstaclesSpawned == 2) {
                    // 2nd obstacle: always mid-level ('g') so the player sees obstacle variety immediately
                    dinoTrack[3] = 3;
                } else {
                    if (dinoObstaclesSpawned == 3) {
                        dinoSpeedupStartMs = dinoTotalPlayMs;
                    }
                    // 3rd+ obstacle: mix of ground (1), mid-level (3), and upper-level (4) — never 2-row (2)
                    int r = random(100);
                    if (r < 50) dinoTrack[3] = 1;      // Ground ('d')
                    else if (r < 80) dinoTrack[3] = 3; // Mid-level ('g')
                    else dinoTrack[3] = 4;             // Upper-level ('a')
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
            // Ducking flat on bottom segment 'd'
            dinoBits = 0x08;
        } else {
            // Running leg animation (alternating e=0x10 and c=0x04)
            uint8_t leg = ((now / 260) % 2 == 0) ? 0x10 : 0x04;
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

    // Lock 2N7000 MOSFET gate pins LOW with internal 45kΩ pull-down held during Deep Sleep
    // so external 100kΩ Gate-to-Source pull-downs are not required
    const int hudPins[3] = {Pins::GreenLed1, Pins::GreenLed2, Pins::GreenLed3};
    for (int p : hudPins) {
        ledcDetachPin(p);
        pinMode(p, OUTPUT);
        digitalWrite(p, LOW);
        gpio_num_t gp = (gpio_num_t)digitalPinToGPIONumber(p);
        gpio_pullup_dis(gp);
        gpio_pulldown_en(gp);
        gpio_hold_en(gp);
    }
    gpio_deep_sleep_hold_en();
    
    // Wait for button release before sleeping so a held button doesn't immediately re-wake
    uint32_t waitStart = millis();
    while ((digitalRead(Pins::PowerButton) == LOW || digitalRead(Pins::JoystickSw) == LOW) && (millis() - waitStart < 2000)) {
        delay(20);
    }
    delay(50);

    // Keep RTC_PERIPH powered in Deep Sleep so the internal pull-up on D33 (PowerButton) stays active!
    gpio_num_t wakePin = (gpio_num_t)digitalPinToGPIONumber(Pins::PowerButton);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    rtc_gpio_pullup_en(wakePin);
    rtc_gpio_pulldown_dis(wakePin);

    Serial.println("Entering Deep Sleep...");
    Serial.flush();

    esp_sleep_enable_ext0_wakeup(wakePin, 0); 
    esp_deep_sleep_start();
}
