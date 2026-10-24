import os
import subprocess
import struct

TARGET_PEAK = 114  # Range strictly bounded within [14, 242], leaving 14 LSB headroom to guarantee ZERO clipping.

def normalize_pcm16_to_u8(raw_bytes, target_peak=TARGET_PEAK):
    num_samples = len(raw_bytes) // 2
    if num_samples == 0:
        return b""
    samples = struct.unpack(f"<{num_samples}h", raw_bytes)
    mean_val = sum(samples) / num_samples
    zero_mean = [s - mean_val for s in samples]
    max_amp = max(abs(s) for s in zero_mean)
    
    gain = (target_peak * 256.0) / max_amp if max_amp > 0 else 1.0
    min_b = 128 - target_peak
    max_b = 128 + target_peak
    
    out = bytearray(num_samples)
    for i, s in enumerate(zero_mean):
        v = int(round((s * gain) / 256.0)) + 128
        if v < min_b: v = min_b
        elif v > max_b: v = max_b
        out[i] = v
    return bytes(out)

SOURCES = [
    ('evil_morty', 'soundbites/evil_morty/Evil Morty Final Theme.mp3', 'soundbites/raw_temp/evil_morty.raw', ['-ss', '00:00:32', '-t', '81']),
    ('roulette_spin', 'soundbites/roullette/Click-click-click.m4a', 'soundbites/raw_temp/roulette_spin.raw', []),
    ('roulette_clack', 'soundbites/roullette/Clank.m4a', 'soundbites/raw_temp/roulette_clack.raw', []),
    ('roulette_boom', 'soundbites/roullette/Boom.m4a', 'soundbites/raw_temp/roulette_boom.raw', []),
    ('roulette_wrecked', 'soundbites/roullette/Wrecked_Son.m4a', 'soundbites/raw_temp/roulette_wrecked.raw', [])
]

ffmpeg_bin = 'tools/ffmpeg.exe' if os.path.exists('tools/ffmpeg.exe') else 'ffmpeg'

# Check for Snake Jazz in soundbites/snake_jazz
os.makedirs('soundbites/snake_jazz', exist_ok=True)
snake_jazz_dir = 'soundbites/snake_jazz'
user_snake_files = [f for f in os.listdir(snake_jazz_dir) if f.lower().endswith(('.mp3', '.m4a', '.wav')) and f != 'snake_jazz_synth.wav']

if user_snake_files:
    user_file = os.path.join(snake_jazz_dir, user_snake_files[0])
    print(f"Found user Snake Jazz track: {user_file}")
    # User's snake_jazz.mp3 is the official 1-minute YouTube release.
    # The theme is an iconic 8-bar loop (6.8571s) starting at 0.2054s.
    SOURCES.append(('snake_jazz', user_file, 'soundbites/raw_temp/snake_jazz.raw', 
                    ['-ss', '0.2054', '-t', '6.8571']))
elif os.path.exists('soundbites/snake_jazz/snake_jazz_synth.wav'):
    SOURCES.append(('snake_jazz', 'soundbites/snake_jazz/snake_jazz_synth.wav', 'soundbites/raw_temp/snake_jazz.raw', []))

# Check for Dino Song in soundbites/dino_game
os.makedirs('soundbites/dino_game', exist_ok=True)
dino_dir = 'soundbites/dino_game'
user_dino_files = [f for f in os.listdir(dino_dir) if f.lower().endswith(('.mp3', '.m4a', '.wav'))]

if user_dino_files:
    dino_file = os.path.join(dino_dir, user_dino_files[0])
    print(f"Found user Dino track: {dino_file}")
    # Trim first 9 seconds and last 5 seconds (total ~82.29s -> 68.2857s duration)
    SOURCES.append(('dino_song', dino_file, 'soundbites/raw_temp/dino_song.raw',
                    ['-ss', '9', '-t', '68.2857']))

os.makedirs('soundbites/raw_temp', exist_ok=True)

# Convert any missing or modified source files using ffmpeg and normalize
temp_16le = 'soundbites/raw_temp/temp_16le.raw'
for item in SOURCES:
    name, src, raw = item[0], item[1], item[2]
    in_opts = item[3] if len(item) > 3 else []
    if not os.path.exists(src):
        raise FileNotFoundError(f"Required audio source file missing: {src}")
    
    print(f"Converting & Normalizing {src} -> {raw}...")
    cmd = [ffmpeg_bin, '-y'] + in_opts + ['-i', src, '-ac', '1', '-ar', '11025', '-f', 's16le', temp_16le]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    with open(temp_16le, 'rb') as fp:
        raw_16 = fp.read()
    if os.path.exists(temp_16le):
        os.remove(temp_16le)
        
    u8_data = normalize_pcm16_to_u8(raw_16)
    with open(raw, 'wb') as fp:
        fp.write(u8_data)
    print(f"  Normalized {name}: {len(u8_data)} bytes, min={min(u8_data)}, max={max(u8_data)} (Target [{128-TARGET_PEAK}, {128+TARGET_PEAK}], Zero Clipping)")

files = [(item[0], item[2]) for item in SOURCES]

h_out = '#pragma once\n#include <stdint.h>\n#include <stddef.h>\n\n'
for name, p in files:
    h_out += f'extern const uint8_t snd_{name}[];\n'
    h_out += f'extern const size_t snd_{name}_size;\n\n'

with open('include/extra_audio.h', 'w') as f:
    f.write(h_out)

with open('src/modules/extra_audio.cpp', 'w') as f:
    f.write('#include "extra_audio.h"\n#include <Arduino.h>\n\n')
    for name, p in files:
        with open(p, 'rb') as fp:
            data = fp.read()
        print(f"Packing snd_{name}: {len(data)} bytes...")
        if name == 'dino_song':
            # Original ESP32 chip (used in Wokwi simulator) has a strict 4MB hardware MMU DROM limit.
            # Real hardware (Arduino Nano ESP32 / ESP32-S3) has full 16MB flash MMU support without this limit.
            wokwi_data = data[:11025] # 1.0 second loop for Wokwi simulation
            f.write('// Wokwi simulation on ESP32 DevKitC has a 4MB hardware MMU DROM limit.\n')
            f.write('// Real hardware (Arduino Nano ESP32 / ESP32-S3) has 16MB flash and uses the full track.\n')
            f.write('#ifdef WOKWI_SIM\n')
            f.write(f'const size_t snd_{name}_size = {len(wokwi_data)};\n')
            f.write(f'const uint8_t snd_{name}[] PROGMEM = {{\n')
            lines = []
            for i in range(0, len(wokwi_data), 16):
                chunk = wokwi_data[i:i+16]
                lines.append('    ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
            f.writelines(lines)
            f.write('};\n#else\n')
            f.write(f'const size_t snd_{name}_size = {len(data)};\n')
            f.write(f'const uint8_t snd_{name}[] PROGMEM = {{\n')
            lines = []
            for i in range(0, len(data), 16):
                chunk = data[i:i+16]
                lines.append('    ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
                if len(lines) >= 1000:
                    f.writelines(lines)
                    lines = []
            if lines:
                f.writelines(lines)
            f.write('};\n#endif\n\n')
        else:
            f.write(f'const size_t snd_{name}_size = {len(data)};\n')
            f.write(f'const uint8_t snd_{name}[] PROGMEM = {{\n')
            lines = []
            for i in range(0, len(data), 16):
                chunk = data[i:i+16]
                lines.append('    ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
                if len(lines) >= 1000:
                    f.writelines(lines)
                    lines = []
            if lines:
                f.writelines(lines)
            f.write('};\n\n')

print('Generated extra_audio.h and extra_audio.cpp successfully!')

