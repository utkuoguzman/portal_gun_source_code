#include "audio.h"
#include "audio_assets.h"
#include "pins.h"
#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_PORT I2S_NUM_0

AudioSystem audio;

static const int8_t IMA_INDEX_TABLE[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static const int16_t IMA_STEP_TABLE[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static inline int16_t decodeImaNibble(uint8_t nibble, int32_t& pred, int32_t& stepIdx) {
    int32_t step = IMA_STEP_TABLE[stepIdx];
    int32_t vpdiff = step >> 3;
    if (nibble & 4) vpdiff += step;
    if (nibble & 2) vpdiff += (step >> 1);
    if (nibble & 1) vpdiff += (step >> 2);

    if (nibble & 8) pred -= vpdiff;
    else            pred += vpdiff;

    if (pred > 32767)       pred = 32767;
    else if (pred < -32768) pred = -32768;

    stepIdx += IMA_INDEX_TABLE[nibble & 0x0F];
    if (stepIdx < 0)       stepIdx = 0;
    else if (stepIdx > 88) stepIdx = 88;

    return (int16_t)pred;
}

#ifdef WOKWI_SIM
static uint32_t lastToneTime = 0;
static uint32_t currentToneFreq = 0;
#endif

void AudioSystem::begin() {
#ifdef WOKWI_SIM
    // In Wokwi simulation, route audio to wokwi-buzzer on GPIO 25
    pinMode(25, OUTPUT);
    tone(25, 880, 80); // Audible confirmation chirp on boot!
#else
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 8400,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = Pins::I2sBclk,
        .ws_io_num = Pins::I2sLrc,
        .data_out_num = Pins::I2sData,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    i2s_zero_dma_buffer(I2S_PORT);
#endif
}

void AudioSystem::update() {
    if (playing && currentData != nullptr) {
        size_t totalSamples = currentSize * 2; // 2 samples per byte (4-bit IMA ADPCM)
        uint32_t elapsed = millis() - playStartTime;
        size_t targetPos = (elapsed * 8400) / 1000;
        
        if (targetPos > totalSamples) {
            targetPos = totalSamples;
        }
        
#ifdef WOKWI_SIM
        currentPos = targetPos;
        uint32_t now = millis();

        // Dedicated ascending sci-fi sweep for the 4-second portal gun power-up
        if (currentData == snd_powerup) {
            if (now - lastToneTime >= 35) {
                lastToneTime = now;
                uint32_t freq = 220 + (elapsed * 900) / 4000;
                if (freq > 1150) freq = 1150;
                if (abs((int)freq - (int)currentToneFreq) > 15 || currentToneFreq == 0) {
                    tone(25, freq);
                    currentToneFreq = freq;
                }
            }
        }
#else
        if (targetPos > currentPos) {
            size_t samples_to_write = targetPos - currentPos;
            const size_t CHUNK_SIZE = 256; 
            if (samples_to_write > CHUNK_SIZE) samples_to_write = CHUNK_SIZE;
            
            // Scaled volume with hardware-matched safety headroom:
            // User volume [0.0 .. 1.0] maps to [0.0 .. MAX_SAFE_HEADROOM] (0.84 = -1.5 dBFS).
            // Guarantees the MAX98357A (Gain=9dB, VDD=5V) never clips into the power rails.
            float effectiveVolume = currentVolume * MAX_SAFE_HEADROOM;
            
            int16_t buffer[CHUNK_SIZE];
            for (size_t i = 0; i < samples_to_write; i++) {
                size_t sampleIdx = currentPos + i;
                uint8_t byteVal = pgm_read_byte(&currentData[sampleIdx >> 1]);
                uint8_t nibble = (sampleIdx & 1) ? ((byteVal >> 4) & 0x0F) : (byteVal & 0x0F);
                int16_t decoded_pcm16 = decodeImaNibble(nibble, adpcmPred, adpcmStepIdx);
                int32_t sample_val = (int32_t)(decoded_pcm16 * effectiveVolume);
                
                // Hardware protection clamp: brickwall anti-clipping limiter
                // Prevents 16-bit integer overflow and rail saturation
                if (sample_val > MAX_UNCLIPPED_AMPLITUDE) {
                    sample_val = MAX_UNCLIPPED_AMPLITUDE;
                } else if (sample_val < -MAX_UNCLIPPED_AMPLITUDE) {
                    sample_val = -MAX_UNCLIPPED_AMPLITUDE;
                }
                buffer[i] = (int16_t)sample_val;
            }
            
            size_t bytes_written = 0;
            i2s_write(I2S_PORT, buffer, samples_to_write * sizeof(int16_t), &bytes_written, 0); 
            currentPos += samples_to_write; 
        }
#endif
        
        if (currentPos >= totalSamples) {
            if (isLooping) {
                currentPos = 0;
                adpcmPred = 0;
                adpcmStepIdx = 0;
                playStartTime = millis();
            } else {
                playing = false;
                currentData = nullptr;
#ifdef WOKWI_SIM
                noTone(25);
                currentToneFreq = 0;
#endif
            }
        }
    }
}

void AudioSystem::playRaw(const uint8_t* data, size_t size, bool loop) {
    if (data == nullptr || size == 0) return;
    currentData = data;
    currentSize = size;
    currentPos = 0;
    adpcmPred = 0;
    adpcmStepIdx = 0;
    playStartTime = millis();
    playing = true;
    isLooping = loop;
    Serial.println("Audio: Playing ADPCM stream");
#ifndef WOKWI_SIM
    i2s_zero_dma_buffer(I2S_PORT);
#endif
}

void AudioSystem::stop() {
    playing = false;
    isLooping = false;
    currentData = nullptr;
#ifdef WOKWI_SIM
    noTone(25);
    currentToneFreq = 0;
#else
    i2s_zero_dma_buffer(I2S_PORT);
#endif
    Serial.println("Audio: Stopped");
}

bool AudioSystem::isPlaying() const {
    return playing;
}

void AudioSystem::setVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    currentVolume = volume;
}

float AudioSystem::getVolume() const {
    return currentVolume;
}

uint32_t AudioSystem::getPositionMs() const {
    if (!playing) return 0;
    return millis() - playStartTime;
}
