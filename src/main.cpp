#include <Arduino.h>
#include "input.h"
#include "leds.h"
#include "display.h"
#include "audio.h"
#include "portal_gun.h"

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("Portal Gun Boot");
    Serial.println("ESP32 initialized");
    
    input.begin();
    Serial.println("Joystick initialized");
    
    display.begin();
    Serial.println("Display initialized");
    
    leds.begin();
    Serial.println("LED system initialized");
    
    audio.begin();
    Serial.println("Audio initialized");
    
    portalGun.begin();
}

void loop() {
    input.update();
    portalGun.update();
    leds.update();
    display.update();
    audio.update();
}
