#include "audio.h"
#include "audio_assets.h"
#include "pins.h"
#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_PORT I2S_NUM_0

AudioSystem audio;

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
        .sample_rate = 11025,
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
        uint32_t elapsed = millis() - playStartTime;
        size_t targetPos = (elapsed * 11025) / 1000;
        
        if (targetPos > currentSize) {
            targetPos = currentSize;
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
        } else {
            // General sound effects (Rick quotes, roulette, blasts, etc.)
            // Rate-limit tone frequency changes to every 40ms to avoid restarting the timer every frame!
            if (now - lastToneTime >= 40) {
                lastToneTime = now;
                
                size_t sampleIndex = (currentPos >= 256) ? (currentPos - 256) : 0;
                size_t window = (currentPos - sampleIndex);
                
                if (window >= 64) {
                    uint8_t min_v = 255, max_v = 0;
                    size_t crossings = 0;
                    for (size_t i = 0; i < window; i++) {
                        uint8_t v = currentData[sampleIndex + i];
                        if (v < min_v) min_v = v;
                        if (v > max_v) max_v = v;
                        if (i > 0) {
                            uint8_t prev = currentData[sampleIndex + i - 1];
                            if ((prev < 128 && v >= 128) || (prev >= 128 && v < 128)) {
                                crossings++;
                            }
                        }
                    }
                    uint8_t amp = max_v - min_v;
                    
                    if (amp < 16 || currentVolume < 0.05f) {
                        if (currentToneFreq != 0) {
                            noTone(25);
                            currentToneFreq = 0;
                        }
                    } else {
                        uint32_t freq = (crossings * 11025) / (2 * window);
                        if (freq < 150) freq = 150;
                        if (freq > 2600) freq = 2600;
                        
                        if (abs((int)freq - (int)currentToneFreq) > 30 || currentToneFreq == 0) {
                            tone(25, freq);
                            currentToneFreq = freq;
                        }
                    }
                }
            }
        }
#else
        if (targetPos > currentPos) {
            size_t samples_to_write = targetPos - currentPos;
            const size_t CHUNK_SIZE = 256; 
            if (samples_to_write > CHUNK_SIZE) samples_to_write = CHUNK_SIZE;
            
            int16_t buffer[CHUNK_SIZE];
            for (size_t i = 0; i < samples_to_write; i++) {
                int8_t signed_sample = (int8_t)((int)currentData[currentPos + i] - 128);
                buffer[i] = (int16_t)(signed_sample * 256.0f * currentVolume);
            }
            
            size_t bytes_written = 0;
            i2s_write(I2S_PORT, buffer, samples_to_write * sizeof(int16_t), &bytes_written, 0); 
            currentPos += samples_to_write; 
        }
#endif
        
        if (currentPos >= currentSize) {
            if (isLooping) {
                currentPos = 0;
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
    playStartTime = millis();
    playing = true;
    isLooping = loop;
    Serial.println("Audio: Playing raw PCM");
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
    currentVolume = volume;
    if (currentVolume < 0.0f) currentVolume = 0.0f;
    if (currentVolume > 1.0f) currentVolume = 1.0f;
}

uint32_t AudioSystem::getPositionMs() const {
    if (!playing) return 0;
    return millis() - playStartTime;
}
