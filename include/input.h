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
    bool doubleClicked;

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
    const unsigned long debounceDelay = 50;
    const unsigned long doubleClickDelay = 400; // ms window for double click
};

extern InputSystem input;
