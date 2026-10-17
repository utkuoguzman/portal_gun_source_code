import os
import subprocess

SOURCES = [
    ('evil_morty', 'soundbites/evil_morty/Evil Morty Final Theme.mp3', 'soundbites/raw_temp/evil_morty.raw', ['-ss', '00:00:32', '-t', '81'], []),
    ('roulette_spin', 'soundbites/roullette/Click-click-click.m4a', 'soundbites/raw_temp/roulette_spin.raw', [], ['-af', 'volume=2.2']),
    ('roulette_clack', 'soundbites/roullette/Clank.m4a', 'soundbites/raw_temp/roulette_clack.raw', [], []),
    ('roulette_boom', 'soundbites/roullette/Boom.m4a', 'soundbites/raw_temp/roulette_boom.raw', [], []),
    ('roulette_wrecked', 'soundbites/roullette/Wrecked_Son.m4a', 'soundbites/raw_temp/roulette_wrecked.raw', [], ['-af', 'volume=2.5'])
]

# Check for Snake Jazz in soundbites/snake_jazz
os.makedirs('soundbites/snake_jazz', exist_ok=True)
snake_jazz_dir = 'soundbites/snake_jazz'
user_snake_files = [f for f in os.listdir(snake_jazz_dir) if f.lower().endswith(('.mp3', '.m4a', '.wav')) and f != 'snake_jazz_synth.wav']

if user_snake_files:
    user_file = os.path.join(snake_jazz_dir, user_snake_files[0])
    print(f"Found user Snake Jazz track: {user_file}")
    # User's snake_jazz.mp3 is the official 1-minute YouTube release.
    # The theme is an iconic 8-bar loop (6.8571s) starting at 0.2054s.
    # Trimming to the exact 8-bar beat boundary gives an unbroken, mathematically seamless loop
    # that repeats indefinitely in-game while fitting cleanly inside ESP32 flash.
    SOURCES.append(('snake_jazz', user_file, 'soundbites/raw_temp/snake_jazz.raw', 
                    ['-ss', '0.2054', '-t', '6.8571'], 
                    ['-af', 'volume=3.5']))
elif os.path.exists('soundbites/snake_jazz/snake_jazz_synth.wav'):
    SOURCES.append(('snake_jazz', 'soundbites/snake_jazz/snake_jazz_synth.wav', 'soundbites/raw_temp/snake_jazz.raw', [], []))

os.makedirs('soundbites/raw_temp', exist_ok=True)

# Convert any missing or modified source files using ffmpeg
for name, src, raw, in_opts, out_opts in SOURCES:
    if not os.path.exists(src):
        raise FileNotFoundError(f"Required audio source file missing: {src}")
    if not os.path.exists(raw) or os.path.getmtime(src) > os.path.getmtime(raw) or os.path.getsize(raw) == 0:
        print(f"Converting {src} -> {raw}...")
        cmd = ['ffmpeg', '-y'] + in_opts + ['-i', src] + out_opts + ['-ac', '1', '-ar', '11025', '-f', 'u8', raw]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

files = [(name, raw) for name, src, raw, in_opts, out_opts in SOURCES]

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

