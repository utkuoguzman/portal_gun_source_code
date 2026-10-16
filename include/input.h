#pragma once

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
    bool doubleClicked;
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

private:
    JoystickState state;
    bool lastButtonState = true;
    bool lastPowerButtonState = true;
    unsigned long lastDebounceTime = 0;
    unsigned long lastPowerDebounceTime = 0;
    unsigned long lastClickTime = 0;
    unsigned long pressStartTime = 0;
    bool holdReported = false;
    const unsigned long debounceDelay = 50;
    const unsigned long doubleClickDelay = 400; // ms window for double click
};

extern InputSystem input;
