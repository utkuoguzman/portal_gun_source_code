#include "input.h"
#include "pins.h"
#include "config.h"
#include <Arduino.h>

InputSystem input;

void InputSystem::begin() {
    pinMode(Pins::JoystickSw, INPUT_PULLUP);
    pinMode(Pins::PowerButton, INPUT_PULLUP);
    
    // Explicitly enforce 12-bit ADC resolution (0-4095) across all boards
    analogReadResolution(12);
    lastReleaseTime = 0;
    pendingClicks = 0;
    
    // Initial read to set states
    state.rawX = analogRead(Pins::JoystickX);
    state.rawY = analogRead(Pins::JoystickY);
    state.x = 0.0f;
    state.y = 0.0f;
    state.left = false;
    state.right = false;
    state.up = false;
    state.down = false;
    state.pressed = false;
    state.justPressed = false;
    state.justReleased = false;
    state.clicked = false;
    state.singleClicked = false;
    state.doubleClicked = false;
    state.tripleClicked = false;
    state.held3s = false;
    state.justHeld3s = false;
    pressStartTime = 0;
    holdReported = false;
    state.powerPressed = false;
    state.powerJustPressed = false;

    lastButtonState = digitalRead(Pins::JoystickSw) == LOW;
    if (lastButtonState) {
        state.pressed = true;
        pressStartTime = millis();
    }

    lastPowerButtonState = digitalRead(Pins::PowerButton) == LOW;
    if (lastPowerButtonState) {
        state.powerPressed = true;
    }
}

void InputSystem::update() {
    // Read ADC
    state.rawX = analogRead(Pins::JoystickX);
    state.rawY = analogRead(Pins::JoystickY);

    // Normalize X (-1.0 to 1.0)
    float nx = (float)(state.rawX - Config::JOYSTICK_X_CENTER) / 2048.0f;
    if (nx > 1.0f) nx = 1.0f;
    if (nx < -1.0f) nx = -1.0f;
    
    // Normalize Y (-1.0 to 1.0)
    float ny = (float)(state.rawY - Config::JOYSTICK_Y_CENTER) / 2048.0f;
    if (ny > 1.0f) ny = 1.0f;
    if (ny < -1.0f) ny = -1.0f;

    // Apply deadzone
    if (fabsf(nx) < Config::JOYSTICK_DEADZONE) nx = 0.0f;
    if (fabsf(ny) < Config::JOYSTICK_DEADZONE) ny = 0.0f;

    state.x = nx;
    state.y = ny;

    state.left = state.x < -0.5f;
    state.right = state.x > 0.5f;
    state.up = state.y > 0.5f;
    state.down = state.y < -0.5f;

    // Debounce button
    bool reading = digitalRead(Pins::JoystickSw) == LOW; // LOW means pressed due to pullup
    
    state.justPressed = false;
    state.justReleased = false;
    state.clicked = false;
    state.singleClicked = false;
    state.doubleClicked = false;
    state.tripleClicked = false;
    state.justHeld3s = false;

    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != state.pressed) {
            state.pressed = reading;
            if (state.pressed) {
                state.justPressed = true;
                pressStartTime = millis();
                holdReported = false;
                
                // Track click sequence
                pendingClicks++;
                if (pendingClicks >= 3) {
                    state.tripleClicked = true;
                    pendingClicks = 0;
                    lastReleaseTime = 0;
                }
            } else {
                state.justReleased = true;
                lastReleaseTime = millis();
                if (!holdReported) {
                    state.clicked = true;
                } else {
                    pendingClicks = 0;
                }
                state.held3s = false;
                holdReported = false;
            }
        }
    }

    // Resolve single or double click after release when no subsequent click occurs
    if (!state.pressed && pendingClicks > 0 && lastReleaseTime > 0) {
        if (millis() - lastReleaseTime >= multiClickDelay) {
            if (pendingClicks == 1) {
                state.singleClicked = true;
            } else if (pendingClicks == 2) {
                state.doubleClicked = true;
            }
            pendingClicks = 0;
            lastReleaseTime = 0;
        }
    }

    // 3-second continuous hold detection
    if (state.pressed) {
        if (!holdReported && (millis() - pressStartTime >= 3000)) {
            state.held3s = true;
            state.justHeld3s = true;
            holdReported = true;
            pendingClicks = 0;
        }
    } else {
        state.held3s = false;
    }

    lastButtonState = reading;

    // Debounce power button
    bool p_reading = digitalRead(Pins::PowerButton) == LOW;
    state.powerJustPressed = false;

    if (p_reading != lastPowerButtonState) {
        lastPowerDebounceTime = millis();
    }

    if ((millis() - lastPowerDebounceTime) > debounceDelay) {
        if (p_reading != state.powerPressed) {
            state.powerPressed = p_reading;
            if (state.powerPressed) {
                state.powerJustPressed = true;
            }
        }
    }

    lastPowerButtonState = p_reading;
}

const JoystickState& InputSystem::getState() const {
    return state;
}

void InputSystem::clearClicks() {
    pendingClicks = 0;
    lastReleaseTime = 0;
    state.clicked = false;
    state.singleClicked = false;
    state.doubleClicked = false;
    state.tripleClicked = false;
}
