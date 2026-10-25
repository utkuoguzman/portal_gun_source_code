#include "display.h"
#include "pins.h"
#include <Arduino.h>

DisplaySystem display;

// Active pin assignments (auto-adapts if user swapped CLK and DIO)
static int activeClkPin = Pins::DisplayClk;
static int activeDioPin = Pins::DisplayDio;

static void tm_delay() {
    delayMicroseconds(100);
}

static void tm_start(int clk, int dio) {
    pinMode(dio, OUTPUT);
    pinMode(clk, OUTPUT);
    digitalWrite(dio, HIGH);
    digitalWrite(clk, HIGH);
    tm_delay();
    digitalWrite(dio, LOW);
    tm_delay();
    digitalWrite(clk, LOW);
    tm_delay();
}

static void tm_stop(int clk, int dio) {
    pinMode(dio, OUTPUT);
    pinMode(clk, OUTPUT);
    digitalWrite(dio, LOW);
    digitalWrite(clk, LOW);
    tm_delay();
    digitalWrite(clk, HIGH);
    tm_delay();
    digitalWrite(dio, HIGH);
    tm_delay();
}

static bool tm_writeByte(int clk, int dio, uint8_t b) {
    pinMode(clk, OUTPUT);
    for (int i = 0; i < 8; i++) {
        digitalWrite(clk, LOW);
        tm_delay();
        digitalWrite(dio, (b & 0x01) ? HIGH : LOW);
        tm_delay();
        digitalWrite(clk, HIGH);
        tm_delay();
        b >>= 1;
    }

    // ACK cycle
    digitalWrite(clk, LOW);
    pinMode(dio, INPUT_PULLUP);
    tm_delay();
    digitalWrite(clk, HIGH);
    tm_delay();
    bool ack = (digitalRead(dio) == LOW);
    digitalWrite(clk, LOW);
    tm_delay();
    pinMode(dio, OUTPUT);
    return ack;
}

static void tm_sendCommand(uint8_t cmd) {
    tm_start(activeClkPin, activeDioPin);
    tm_writeByte(activeClkPin, activeDioPin, cmd);
    tm_stop(activeClkPin, activeDioPin);
}

static void tm_writeSegments(const uint8_t segments[4]) {
    // Command 1: Data write, auto increment address
    tm_sendCommand(0x40);

    // Command 2: Set start address to 0xC0 (first digit)
    tm_start(activeClkPin, activeDioPin);
    tm_writeByte(activeClkPin, activeDioPin, 0xC0);
    for (int i = 0; i < 4; i++) {
        tm_writeByte(activeClkPin, activeDioPin, segments[i]);
    }
    tm_stop(activeClkPin, activeDioPin);

    // Command 3: Display ON with max brightness (0x88 | 0x07 = 0x8F)
    tm_sendCommand(0x8F);
}

void DisplaySystem::begin() {
    pinMode(Pins::DisplayClk, OUTPUT);
    pinMode(Pins::DisplayDio, OUTPUT);
    digitalWrite(Pins::DisplayClk, HIGH);
    digitalWrite(Pins::DisplayDio, HIGH);
    delay(20);

    // Test 1: Standard wiring (D7 = CLK, D8 = DIO)
    tm_start(Pins::DisplayClk, Pins::DisplayDio);
    bool ack1 = tm_writeByte(Pins::DisplayClk, Pins::DisplayDio, 0x40);
    tm_stop(Pins::DisplayClk, Pins::DisplayDio);

    if (ack1) {
        activeClkPin = Pins::DisplayClk;
        activeDioPin = Pins::DisplayDio;
        Serial.println("[Display] TM1637 ACK OK on standard pins (D7=CLK, D8=DIO).");
    } else {
        // Test 2: Inverted wiring (D8 = CLK, D7 = DIO)
        tm_start(Pins::DisplayDio, Pins::DisplayClk);
        bool ack2 = tm_writeByte(Pins::DisplayDio, Pins::DisplayClk, 0x40);
        tm_stop(Pins::DisplayDio, Pins::DisplayClk);

        if (ack2) {
            activeClkPin = Pins::DisplayDio;
            activeDioPin = Pins::DisplayClk;
            Serial.println("[Display] TM1637 ACK OK on reversed pins (D8=CLK, D7=DIO)! Auto-swapped in software.");
        } else {
            activeClkPin = Pins::DisplayClk;
            activeDioPin = Pins::DisplayDio;
            Serial.println("[Display] WARNING: TM1637 did not ACK on D7/D8. Check 5V/3.3V power, GND, and wiring.");
        }
    }

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
        case 'K': return 0x75;
        case 'L': return 0x38;
        case 'M': return 0x15;
        case 'N': return 0x54; // n
        case 'O': return 0x3F;
        case 'P': return 0x73;
        case 'Q': return 0x67;
        case 'R': return 0x50; // r
        case 'S': return 0x6D;
        case 'T': return 0x78; // t
        case 'U': return 0x3E;
        case 'V': return 0x1C; // v
        case 'W': return 0x2A;
        case 'X': return 0x76;
        case 'Y': return 0x6E;
        case 'Z': return 0x5B;
        case ' ': return 0x00;
        case '-': return 0x40;
        case '!': return 0x86;
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
        tm_writeSegments(data);
        
        scrollPos++;
        if (scrollPos > scrollLength) {
            isScrolling = false;
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
    char buf[5];
    snprintf(buf, sizeof(buf), "%4d", value);
    showText(buf);
}

void DisplaySystem::showText(const char* text) {
    isScrolling = false;
    uint8_t data[4] = {0, 0, 0, 0};
    int len = strlen(text);
    for (int i = 0; i < 4 && i < len; i++) {
        data[i] = encodeChar(text[i]);
    }
    tm_writeSegments(data);
}

void DisplaySystem::showSegments(const uint8_t segments[]) {
    isScrolling = false;
    tm_writeSegments(segments);
}

void DisplaySystem::showColon(bool show) {
    (void)show;
}

void DisplaySystem::clear() {
    uint8_t data[4] = {0, 0, 0, 0};
    tm_writeSegments(data);
}
