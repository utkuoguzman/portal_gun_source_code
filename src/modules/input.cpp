#include "input.h"
#include "pins.h"
#include "config.h"
#include <Arduino.h>

InputSystem input;

void InputSystem::begin() {
    pinMode(Pins::JoystickSw, INPUT_PULLUP);
    pinMode(Pins::PowerButton, INPUT_PULLUP);
    
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
    state.powerPressed = false;
    state.powerJustPressed = false;

    lastButtonState = digitalRead(Pins::JoystickSw) == LOW;
    if (lastButtonState) {
        state.pressed = true;
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
    if (abs(nx) < Config::JOYSTICK_DEADZONE) nx = 0.0f;
    if (abs(ny) < Config::JOYSTICK_DEADZONE) ny = 0.0f;

    state.x = nx;
    state.y = ny;

    state.left = state.x < -0.5f;
    state.right = state.x > 0.5f;
    state.down = state.y > 0.5f; // Assuming larger values are down/right, adjust as needed
    state.up = state.y < -0.5f;

    // Debounce button
    bool reading = digitalRead(Pins::JoystickSw) == LOW; // LOW means pressed due to pullup
    
    state.justPressed = false;
    state.justReleased = false;
    state.doubleClicked = false;

    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != state.pressed) {
            state.pressed = reading;
            if (state.pressed) {
                state.justPressed = true;
                
                // Double click detection
                if (millis() - lastClickTime < doubleClickDelay) {
                    state.doubleClicked = true;
                    lastClickTime = 0; // reset
                } else {
                    lastClickTime = millis();
                }
            } else {
                state.justReleased = true;
            }
        }
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
