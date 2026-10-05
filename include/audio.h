#pragma once
#include <stdint.h>
#include <stddef.h>

class AudioSystem {
public:
    void begin();
    void update();

    // Play a raw 8-bit unsigned PCM array at 22050 Hz
    void playRaw(const uint8_t* data, size_t size);
    void stop();
    bool isPlaying() const;
    void setVolume(float volume);
    
    // Helper to get playback position in MS
    uint32_t getPositionMs() const;

private:
    const uint8_t* currentData = nullptr;
    size_t currentSize = 0;
    size_t currentPos = 0;
    bool playing = false;
    float currentVolume = 1.0f;
    unsigned long playStartTime = 0;
};

extern AudioSystem audio;
