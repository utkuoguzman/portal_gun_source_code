import os

bg_path = 'site_assets/portal_background.jpg'
gif_path = 'site_assets/rick-dance.gif'

with open(bg_path, 'rb') as f:
    bg_data = f.read()

with open(gif_path, 'rb') as f:
    gif_data = f.read()

print(f"Loaded background: {len(bg_data)} bytes, GIF: {len(gif_data)} bytes")

h_content = """#pragma once
#include <Arduino.h>

extern const uint8_t portal_background_jpg[] PROGMEM;
extern const size_t portal_background_jpg_size;

extern const uint8_t rick_dance_gif[] PROGMEM;
extern const size_t rick_dance_gif_size;
"""

with open('include/site_assets.h', 'w') as f:
    f.write(h_content)

with open('src/modules/site_assets.cpp', 'w') as f:
    f.write('#include "site_assets.h"\n\n')
    f.write(f'const size_t portal_background_jpg_size = {len(bg_data)};\n')
    f.write('const uint8_t portal_background_jpg[] PROGMEM = {\n')
    
    # Write in blocks
    lines = []
    for i in range(0, len(bg_data), 16):
        chunk = bg_data[i:i+16]
        lines.append('    ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
        if len(lines) >= 1000:
            f.writelines(lines)
            lines = []
    if lines:
        f.writelines(lines)
    f.write('};\n\n')

    f.write(f'const size_t rick_dance_gif_size = {len(gif_data)};\n')
    f.write('const uint8_t rick_dance_gif[] PROGMEM = {\n')
    lines = []
    for i in range(0, len(gif_data), 16):
        chunk = gif_data[i:i+16]
        lines.append('    ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
        if len(lines) >= 1000:
            f.writelines(lines)
            lines = []
    if lines:
        f.writelines(lines)
    f.write('};\n')

print("Assets converted successfully to include/site_assets.h and src/modules/site_assets.cpp!")

