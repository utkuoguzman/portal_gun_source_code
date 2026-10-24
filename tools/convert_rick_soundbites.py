import os
import re
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

soundbites_dir = "soundbites/rick_soundbites"
temp_dir = "soundbites/raw_temp"
os.makedirs(temp_dir, exist_ok=True)

# List of files
mp3_files = sorted([f for f in os.listdir(soundbites_dir) if f.endswith('.mp3')])

print(f"Found {len(mp3_files)} MP3 files to convert.")

# Map file names to safe C identifier and 4-char display label
label_map = {
    "Voicy_Pickle Rick!!!.mp3": ("pickle", "PCKL"),
    "Voicy_Rick and Morty - Riggity Riggity Wrecked Son!.mp3": ("riggity", "RIGG"),
    "Voicy_rick and morty.mp3": ("rickmorty", "RAnM"),
    "Voicy_Rick Sanchez Bored.mp3": ("bored", "bOrd"),
    "Voicy_Rick Sanchez Btch.mp3": ("bitch", "btch"),
    "Voicy_Rick Sanchez Butter.mp3": ("butter", "bUtr"),
    "Voicy_Rick Sanchez CPU.mp3": ("cpu", "CPU_"),
    "Voicy_Rick Sanchez Cromulon.mp3": ("cromulon", "CrOm"),
    "Voicy_Rick Sanchez Cute.mp3": ("cute", "CUtE"),
    "Voicy_Rick Sanchez Dance.mp3": ("dance", "dANC"),
    "Voicy_Rick Sanchez Devil_ 2.mp3": ("devil", "dEUL"),
    "Voicy_Rick Sanchez Drink_.mp3": ("drink", "drnk"),
    "Voicy_Rick Sanchez Errand.mp3": ("errand", "ERnd"),
    "Voicy_Rick Sanchez GAF 2.mp3": ("gaf", "GAF_"),
    "Voicy_Rick Sanchez Girls.mp3": ("girls", "GRLS"),
    "Voicy_Rick Sanchez Go back.mp3": ("goback", "bACK"),
    "Voicy_Rick Sanchez Hands.mp3": ("hands", "HndS"),
    "Voicy_Rick Sanchez Ice Cream.mp3": ("icecream", "ICEC"),
    "Voicy_Rick Sanchez Im having.mp3": ("imhaving", "HAUG"),
    "Voicy_Rick Sanchez Know_.mp3": ("know", "knOU"),
    "Voicy_Rick Sanchez Makeout_.mp3": ("makeout", "mAKE"),
    "Voicy_Rick Sanchez Meet.mp3": ("meet", "mEEt"),
    "Voicy_Rick Sanchez Months.mp3": ("months", "mnth"),
    "Voicy_Rick Sanchez More.mp3": ("more", "mOrE"),
    "Voicy_Rick Sanchez Old song.mp3": ("oldsong", "SONG"),
    "Voicy_Rick Sanchez Operation.mp3": ("operation", "OPER"),
    "Voicy_Rick Sanchez OS, MF.mp3": ("osmf", "OSmF"),
    "Voicy_Rick Sanchez OS.mp3": ("os", "OS__"),
    "Voicy_Rick Sanchez Page.mp3": ("page", "PAGE"),
    "Voicy_Rick Sanchez Party.mp3": ("party", "PRtY"),
    "Voicy_Rick Sanchez People.mp3": ("people", "PPL_"),
    "Voicy_Rick Sanchez Player.mp3": ("player", "PLAY"),
    "Voicy_Rick Sanchez Rickless.mp3": ("rickless", "RLES"),
    "Voicy_Rick Sanchez Rubber.mp3": ("rubber", "RUbR"),
    "Voicy_Rick Sanchez Satisfaction.mp3": ("satisfaction", "SAtS"),
    "Voicy_Rick Sanchez Shady.mp3": ("shady", "SHdY"),
    "Voicy_Rick Sanchez Sleep_.mp3": ("sleep", "SLEP"),
    "Voicy_Rick Sanchez Sucks.mp3": ("sucks", "SUCK"),
    "Voicy_Rick Sanchez Sweetie.mp3": ("sweetie", "SWEE"),
    "Voicy_Rick Sanchez Thank you.mp3": ("thankyou", "tHnk"),
    "Voicy_Rick Sanchez Whiskey_.mp3": ("whiskey", "WHSK"),
    "Voicy_Rick Sanchez Y or N.mp3": ("yorn", "YorN"),
    "Voicy_Ricky Tiffy Taffy Bitch.mp3": ("rickytiffy", "tIFF"),
    "Voicy_Wubba Lubba Dub Dub (rick and morty).mp3": ("wubba", "WUBB")
}

ffmpeg_bin = "tools/ffmpeg.exe" if os.path.exists("tools/ffmpeg.exe") else "ffmpeg"
converted_entries = []
temp_16le = os.path.join(temp_dir, "temp_16le.raw")

for mp3 in mp3_files:
    ident, label = label_map.get(mp3, (re.sub(r'[^a-zA-Z0-9]', '_', mp3.replace('.mp3', '')).lower(), "RICK"))
    # Ensure label is exactly 4 chars
    label = (label + "    ")[:4]
    
    in_path = os.path.join(soundbites_dir, mp3)
    out_raw = os.path.join(temp_dir, f"{ident}.raw")
    
    # Run ffmpeg to convert to 11025 Hz, mono, 16-bit signed PCM
    cmd = [
        ffmpeg_bin, "-y", "-i", in_path,
        "-ar", "11025", "-ac", "1", "-f", "s16le", temp_16le
    ]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    
    with open(temp_16le, "rb") as f:
        raw_16 = f.read()
    if os.path.exists(temp_16le):
        os.remove(temp_16le)
        
    raw_bytes = normalize_pcm16_to_u8(raw_16)
    with open(out_raw, "wb") as f:
        f.write(raw_bytes)
        
    converted_entries.append((ident, label, raw_bytes))
    print(f"Converted & Normalized {mp3} -> snd_rick_{ident} ({len(raw_bytes)} bytes, min={min(raw_bytes)}, max={max(raw_bytes)})")

# Write include/soundboard_data.h
with open("include/soundboard_data.h", "w") as f:
    f.write("#pragma once\n#include <stdint.h>\n#include <stddef.h>\n#include \"audio_assets.h\"\n\n")
    for ident, label, raw_bytes in converted_entries:
        f.write(f"extern const uint8_t snd_rick_{ident}[];\n")
        f.write(f"extern const size_t snd_rick_{ident}_size;\n")
    f.write("\nextern const Soundbyte rick_soundboard[];\n")
    f.write("extern const size_t rick_soundboard_count;\n")

# Write src/modules/soundboard_data.cpp
with open("src/modules/soundboard_data.cpp", "w") as f:
    f.write('#include "soundboard_data.h"\n#include <Arduino.h>\n\n')
    
    for ident, label, raw_bytes in converted_entries:
        f.write(f"const size_t snd_rick_{ident}_size = {len(raw_bytes)};\n")
        f.write(f"const uint8_t snd_rick_{ident}[] PROGMEM = {{\n")
        lines = []
        for i in range(0, len(raw_bytes), 16):
            chunk = raw_bytes[i:i+16]
            lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
            if len(lines) >= 1000:
                f.writelines(lines)
                lines = []
        if lines:
            f.writelines(lines)
        f.write("};\n\n")
        
    f.write("// Complete Rick Sanchez Soundboard Table\n")
    f.write("const Soundbyte rick_soundboard[] = {\n")
    f.write('    { snd_buckleup, snd_buckleup_size, "BCKL" },\n')
    for ident, label, raw_bytes in converted_entries:
        f.write(f'    {{ snd_rick_{ident}, snd_rick_{ident}_size, "{label}" }},\n')
    f.write("};\n\n")
    f.write("const size_t rick_soundboard_count = sizeof(rick_soundboard) / sizeof(rick_soundboard[0]);\n")

print(f"Generated {len(converted_entries) + 1} total soundboard entries in soundboard_data.h/cpp!")

