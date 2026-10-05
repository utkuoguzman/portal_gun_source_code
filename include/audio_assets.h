#pragma once
#include <stdint.h>
#include <stddef.h>

struct SubtitleLine {
    uint32_t startTimeMs;
    uint32_t endTimeMs;
    const char* text;
};

extern const uint8_t snd_powerup[];
extern const size_t snd_powerup_size;

extern const uint8_t snd_buckleup[];
extern const size_t snd_buckleup_size;

extern const uint8_t snd_portal_open1[];
extern const size_t snd_portal_open1_size;

extern const uint8_t snd_portal_open2[];
extern const size_t snd_portal_open2_size;

extern const uint8_t snd_portal_close1[];
extern const size_t snd_portal_close1_size;

extern const uint8_t snd_portal_close2[];
extern const size_t snd_portal_close2_size;

extern const uint8_t snd_getschwifty[];
extern const size_t snd_getschwifty_size;

extern const uint8_t env_getschwifty_low[];
extern const size_t env_getschwifty_low_size;

extern const uint8_t env_getschwifty_mid[];
extern const size_t env_getschwifty_mid_size;

extern const uint8_t env_getschwifty_high[];
extern const size_t env_getschwifty_high_size;

extern const SubtitleLine getschwifty_lyrics[];
extern const size_t getschwifty_lyrics_count;

extern const uint8_t snd_powerdown[];
extern const size_t snd_powerdown_size;

