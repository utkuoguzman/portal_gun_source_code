#pragma once
#include <stdint.h>

class DisplaySystem {
public:
    void begin();
    void update();

    void showNumber(int value);
    void showText(const char* text);
    void showSegments(const uint8_t segments[]);
    void showColon(bool show);
    void clear();
    
    // Scrolling text API
    void startScrolling(const char* text, int speedMs);
    void updateScroll();
    bool isScrollFinished() const;
    
    // Custom chars
    uint8_t encodeChar(char c);

private:
    char scrollText[64];
    int scrollLength = 0;
    int scrollPos = 0;
    int scrollSpeedMs = 250;
    unsigned long lastScrollTime = 0;
    bool isScrolling = false;
};

extern DisplaySystem display;
