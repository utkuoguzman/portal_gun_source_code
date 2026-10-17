#pragma once
#include <stdint.h>

struct JoystickState {
    int rawX;
    int rawY;

    float x;
    float y;

    bool left;
    bool right;
    bool up;
    bool down;

    bool pressed;
    bool justPressed;
    bool justReleased;
    bool clicked;
    bool singleClicked;
    bool doubleClicked;
    bool tripleClicked;
    bool held3s;
    bool justHeld3s;

    bool powerPressed;
    bool powerJustPressed;
};

class InputSystem {
public:
    void begin();
    void update();
    const JoystickState& getState() const;
    void clearClicks();

private:
    JoystickState state;
    bool lastButtonState = true;
    bool lastPowerButtonState = true;
    unsigned long lastDebounceTime = 0;
    unsigned long lastPowerDebounceTime = 0;
    unsigned long lastReleaseTime = 0;
    uint8_t pendingClicks = 0;
    unsigned long pressStartTime = 0;
    bool holdReported = false;
    const unsigned long debounceDelay = 50;
    const unsigned long multiClickDelay = 280; // ms window for multi-clicks (single/double/triple)
};

extern InputSystem input;
