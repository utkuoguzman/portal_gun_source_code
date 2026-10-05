#include "audio.h"
#include "pins.h"
#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_PORT I2S_NUM_0

AudioSystem audio;

void AudioSystem::begin() {
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
}

void AudioSystem::update() {
    if (playing && currentData != nullptr) {
        const size_t CHUNK_SIZE = 128; // Process 128 samples at a time to prevent blocking loop too long
        int16_t buffer[CHUNK_SIZE];
        
        size_t samples_to_write = currentSize - currentPos;
        if (samples_to_write > CHUNK_SIZE) {
            samples_to_write = CHUNK_SIZE;
        }
        
        if (samples_to_write > 0) {
            // Convert 8-bit unsigned PCM to 16-bit signed PCM
            for (size_t i = 0; i < samples_to_write; i++) {
                int8_t signed_sample = (int8_t)((int)currentData[currentPos + i] - 128);
                buffer[i] = (int16_t)(signed_sample * 256.0f * currentVolume);
            }
            
            size_t bytes_written = 0;
            esp_err_t err = i2s_write(I2S_PORT, buffer, samples_to_write * sizeof(int16_t), &bytes_written, 0); // 0 ticks = non-blocking
            
            if (err == ESP_OK && bytes_written > 0) {
                currentPos += (bytes_written / sizeof(int16_t));
            }
        }
        
        if (currentPos >= currentSize) {
            playing = false;
            currentData = nullptr;
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
    Serial.println("Audio: Playing raw PCM via I2S");
    i2s_zero_dma_buffer(I2S_PORT);
}

void AudioSystem::stop() {
    playing = false;
    currentData = nullptr;
    i2s_zero_dma_buffer(I2S_PORT);
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
    // Calculate position based on samples handed to DMA
    return (currentPos * 1000) / 11025;
}
