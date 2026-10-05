#include "audio.h"
#include <Arduino.h>

AudioSystem audio;

void AudioSystem::begin() {
    // I2S init will go here for physical hardware
}

void AudioSystem::update() {
    // Track simulated playback time
    if (playing && currentData != nullptr) {
        // At 11025 Hz, 1 byte = 1 sample
        uint32_t elapsed = millis() - playStartTime;
        currentPos = (elapsed * 11025) / 1000;
        
        if (currentPos >= currentSize) {
            playing = false;
        }
    }
}

void AudioSystem::playRaw(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0) return;
    currentData = data;
    currentSize = size;
    currentPos = 0;
    playStartTime = millis();
    playing = true;
    Serial.println("Audio: Playing raw PCM");
}

void AudioSystem::stop() {
    playing = false;
    currentData = nullptr;
    Serial.println("Audio: Stopped");
}

bool AudioSystem::isPlaying() const {
    return playing;
}

void AudioSystem::setVolume(float volume) {
    currentVolume = volume;
    if (currentVolume < 0.0f) currentVolume = 0.0f;
    if (currentVolume > 1.0f) currentVolume = 1.0f;
}

uint32_t AudioSystem::getPositionMs() const {
    if (!playing) return 0;
    return millis() - playStartTime;
}
