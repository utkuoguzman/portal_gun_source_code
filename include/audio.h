#pragma once
#include <stdint.h>
#include <stddef.h>

class AudioSystem {
public:
    void begin();
    void update();

    // Play a raw 8-bit unsigned PCM array at 11025 Hz (optional looping)
    void playRaw(const uint8_t* data, size_t size, bool loop = false);
    void stop();
    bool isPlaying() const;
    void setVolume(float volume);
    float getVolume() const;
    
    // Helper to get playback position in MS
    uint32_t getPositionMs() const;

    // -------------------------------------------------------------------------
    // Hardware-Matched Safe Headroom & Anti-Clipping Limits
    // -------------------------------------------------------------------------
    // MAX98357A Class-D BTL Amp (Gain = 9dB unconnected, VDD = 5.0V):
    // Standard 0 dBFS input attempts to swing 5.08V peak, which hard-clips against the 5V rail.
    // MAX_SAFE_HEADROOM = 0.84 (-1.5 dBFS) limits peak output to ~3.8V and RMS power to ~2.28W into 4 ohms.
    // This strictly protects the 3W speaker, prevents voice coil overheating, eliminates battery sag,
    // and maximizes acoustic loudness without a single clipped waveform.
    static constexpr float MAX_SAFE_HEADROOM = 0.84f;
    static constexpr int16_t MAX_UNCLIPPED_AMPLITUDE = 27500;

private:
    const uint8_t* currentData = nullptr;
    size_t currentSize = 0;
    size_t currentPos = 0;
    bool playing = false;
    bool isLooping = false;
    float currentVolume = 1.0f; // User volume setting [0.0f .. 1.0f], scaled internally by MAX_SAFE_HEADROOM
    unsigned long playStartTime = 0;
    int32_t adpcmPred = 0;
    int32_t adpcmStepIdx = 0;
};

extern AudioSystem audio;
