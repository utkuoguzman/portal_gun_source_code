#include "display.h"
#include "pins.h"
#include <Arduino.h>
#include <TM1637Display.h>

DisplaySystem display;

static TM1637Display tm(Pins::DisplayClk, Pins::DisplayDio);

void DisplaySystem::begin() {
    tm.setBrightness(0x0f); // Max brightness
    clear();
}

uint8_t DisplaySystem::encodeChar(char c) {
    if (c >= 'a' && c <= 'z') c -= 32; // to upper
    switch (c) {
        case 'A': return 0x77;
        case 'B': return 0x7C; // b
        case 'C': return 0x39;
        case 'D': return 0x5E; // d
        case 'E': return 0x79;
        case 'F': return 0x71;
        case 'G': return 0x3D;
        case 'H': return 0x76;
        case 'I': return 0x06;
        case 'J': return 0x1E;
        case 'K': return 0x75; // roughly K
        case 'L': return 0x38;
        case 'M': return 0x15; // roughly M
        case 'N': return 0x54; // n
        case 'O': return 0x3F;
        case 'P': return 0x73;
        case 'Q': return 0x67;
        case 'R': return 0x50; // r
        case 'S': return 0x6D;
        case 'T': return 0x78; // t
        case 'U': return 0x3E;
        case 'V': return 0x1C; // v
        case 'W': return 0x2A; // roughly W
        case 'X': return 0x76; // H-like
        case 'Y': return 0x6E;
        case 'Z': return 0x5B;
        case ' ': return 0x00;
        case '-': return 0x40;
        case '!': return 0x86; // I with dot
        case '1': return 0x06;
        case '2': return 0x5B;
        case '3': return 0x4F;
        case '4': return 0x66;
        case '5': return 0x6D;
        case '6': return 0x7D;
        case '7': return 0x07;
        case '8': return 0x7F;
        case '9': return 0x6F;
        case '0': return 0x3F;
        default:  return 0x00;
    }
}

void DisplaySystem::update() {
    updateScroll();
}

void DisplaySystem::updateScroll() {
    if (!isScrolling || scrollLength == 0) return;
    
    if (millis() - lastScrollTime > scrollSpeedMs) {
        lastScrollTime = millis();
        
        uint8_t data[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            int charIdx = scrollPos + i;
            if (charIdx >= 0 && charIdx < scrollLength) {
                data[i] = encodeChar(scrollText[charIdx]);
            }
        }
        tm.setSegments(data);
        
        scrollPos++;
        if (scrollPos > scrollLength) {
            isScrolling = false; // Stop when finished instead of looping endlessly
        }
    }
}

bool DisplaySystem::isScrollFinished() const {
    return !isScrolling;
}

void DisplaySystem::startScrolling(const char* text, int speedMs) {
    strncpy(scrollText, text, 63);
    scrollText[63] = '\0';
    scrollLength = strlen(scrollText);
    scrollPos = -4; // start offscreen
    scrollSpeedMs = speedMs;
    isScrolling = true;
    lastScrollTime = millis();
}

void DisplaySystem::showNumber(int value) {
    isScrolling = false;
    tm.showNumberDec(value, true); 
}

void DisplaySystem::showText(const char* text) {
    isScrolling = false;
    uint8_t data[4] = {0, 0, 0, 0};
    int len = strlen(text);
    for (int i = 0; i < 4 && i < len; i++) {
        data[i] = encodeChar(text[i]);
    }
    tm.setSegments(data);
}

void DisplaySystem::showSegments(const uint8_t segments[]) {
    isScrolling = false;
    tm.setSegments(segments);
}

void DisplaySystem::showColon(bool show) {
    // TM1637 colon is usually bit 7 of the second digit
    // The library handles this via point parameter in showNumberDecEx
    // This is a stub for the simple interface
}

void DisplaySystem::clear() {
    tm.clear();
}
