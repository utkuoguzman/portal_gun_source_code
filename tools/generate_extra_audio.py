import os
import subprocess

SOURCES = [
    ('evil_morty', 'soundbites/evil_morty/Evil Morty Final Theme.mp3', 'soundbites/raw_temp/evil_morty.raw', ['-ss', '00:00:32', '-t', '81']),
    ('roulette_spin', 'soundbites/roullette/Click-click-click.m4a', 'soundbites/raw_temp/roulette_spin.raw', []),
    ('roulette_clack', 'soundbites/roullette/Clank.m4a', 'soundbites/raw_temp/roulette_clack.raw', []),
    ('roulette_boom', 'soundbites/roullette/Boom.m4a', 'soundbites/raw_temp/roulette_boom.raw', []),
    ('roulette_wrecked', 'soundbites/roullette/Wrecked_Son.m4a', 'soundbites/raw_temp/roulette_wrecked.raw', [])
]

os.makedirs('soundbites/raw_temp', exist_ok=True)

# Convert any missing or modified source files using ffmpeg
for name, src, raw, extra in SOURCES:
    if not os.path.exists(src):
        raise FileNotFoundError(f"Required audio source file missing: {src}")
    if not os.path.exists(raw) or os.path.getmtime(src) > os.path.getmtime(raw):
        print(f"Converting {src} -> {raw}...")
        cmd = ['ffmpeg', '-y'] + extra + ['-i', src, '-ac', '1', '-ar', '11025', '-f', 'u8', raw]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

files = [(name, raw) for name, src, raw, _ in SOURCES]

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

