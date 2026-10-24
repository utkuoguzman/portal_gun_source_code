import os
import re
import wave
import struct

TARGET_DIR = "soundbites/normalized_wavs"
PORTAL_DIR = os.path.join(TARGET_DIR, "01_portal_gun")
THEMES_DIR = os.path.join(TARGET_DIR, "02_games_and_themes")
RICK_DIR = os.path.join(TARGET_DIR, "03_rick_soundboard")

def save_wav(filepath, raw_u8_bytes, sample_rate=11025):
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with wave.open(filepath, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(1)
        f.setframerate(sample_rate)
        f.writeframes(raw_u8_bytes)

def parse_arrays_from_cpp(filepath):
    """Parses const uint8_t <name>[] = { ... }; into a dict of name -> bytes"""
    print(f"Parsing C++ audio arrays from {filepath}...")
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    arrays = {}
    pattern = re.compile(r'const\s+uint8_t\s+(\w+)\[\](?:\s+PROGMEM)?\s*=\s*\{([^}]+)\};')
    for name, hex_data in pattern.findall(content):
        nums = [int(x, 16) for x in re.findall(r'0x[0-9a-fA-F]+', hex_data)]
        b = bytes(nums)
        # Keep the larger one if duplicate (e.g. dino_song hardware vs wokwi)
        if name not in arrays or len(b) > len(arrays[name]):
            arrays[name] = b
    return arrays

def main():
    print("=" * 70)
    print("   EXPORTING NORMALIZED ZERO-CLIPPING AUDIO ARRAYS TO WAV")
    print("=" * 70)

    # 1. Portal gun sounds
    portal_map = {
        "snd_powerup": ("01_powerup.wav", "Portal Gun Startup Power Up"),
        "snd_buckleup": ("02_buckleup.wav", "Buckle Up Bitches!"),
        "snd_portal_open1": ("03_portal_open1.wav", "Portal Blast 1"),
        "snd_portal_open2": ("04_portal_open2.wav", "Portal Blast 2"),
        "snd_portal_close1": ("05_portal_close1.wav", "Portal Close 1"),
        "snd_portal_close2": ("06_portal_close2.wav", "Portal Close 2"),
        "snd_getschwifty": ("07_getschwifty.wav", "Get Schwifty Full Song"),
        "snd_powerdown": ("08_powerdown.wav", "Portal Gun Power Down"),
    }
    portal_arrays = parse_arrays_from_cpp("src/modules/audio_assets.cpp")
    print(f"\n[1/3] Processing Portal Gun Sounds ({len(portal_map)} tracks)...")
    for arr_name, (fname, desc) in portal_map.items():
        if arr_name in portal_arrays:
            data = portal_arrays[arr_name]
            out_p = os.path.join(PORTAL_DIR, fname)
            save_wav(out_p, data)
            pk = max(abs(b - 128) for b in data)
            dur = len(data) / 11025.0
            print(f"  -> {fname:22s} | {dur:5.2f}s | {len(data):7d} B | Peak: {pk}/127 | {desc}")

    # 2. Games and dimension themes
    theme_map = {
        "snd_evil_morty": ("01_evil_morty_theme.wav", "Evil Morty Theme (Dimension M-720)"),
        "snd_roulette_spin": ("02_roulette_spin.wav", "Russian Roulette Spin (Click-click-click)"),
        "snd_roulette_clack": ("03_roulette_clack.wav", "Russian Roulette Safe Strike (Clank)"),
        "snd_roulette_boom": ("04_roulette_boom.wav", "Russian Roulette Live Round (Boom)"),
        "snd_roulette_wrecked": ("05_roulette_wrecked.wav", "Russian Roulette Gloat (Wrecked Son!)"),
        "snd_snake_jazz": ("06_snake_jazz.wav", "Snake Jazz (Dimension S-555)"),
        "snd_dino_song": ("07_dino_song.wav", "Dino Game Theme Song (Dimension D-999)"),
    }
    extra_arrays = parse_arrays_from_cpp("src/modules/extra_audio.cpp")
    print(f"\n[2/3] Processing Games & Dimension Themes ({len(theme_map)} tracks)...")
    for arr_name, (fname, desc) in theme_map.items():
        if arr_name in extra_arrays:
            data = extra_arrays[arr_name]
            out_p = os.path.join(THEMES_DIR, fname)
            save_wav(out_p, data)
            pk = max(abs(b - 128) for b in data)
            dur = len(data) / 11025.0
            print(f"  -> {fname:25s} | {dur:5.2f}s | {len(data):7d} B | Peak: {pk}/127 | {desc}")

    # 3. Rick Soundboard quotes
    rick_arrays = parse_arrays_from_cpp("src/modules/soundboard_data.cpp")
    # Also read table order from soundboard_data.cpp
    with open("src/modules/soundboard_data.cpp", "r", encoding="utf-8") as f:
        sb_txt = f.read()

    table_pattern = re.compile(r'\{\s*(snd_\w+),\s*\w+,\s*"([^"]+)"\s*\}')
    soundboard_entries = table_pattern.findall(sb_txt)
    print(f"\n[3/3] Processing Rick Soundboard Quotes ({len(soundboard_entries)} quotes)...")

    # Friendly names dictionary
    friendly_names = {
        "snd_buckleup": "Buckle Up Bitches!",
        "snd_rick_pickle": "Pickle Rick!!!",
        "snd_rick_bored": "I'm Bored",
        "snd_rick_bitch": "What's Up Bitch?",
        "snd_rick_butter": "What Is My Purpose? (Butter)",
        "snd_rick_cpu": "Central Processing Unit",
        "snd_rick_cromulon": "Show Me What You Got (Cromulon)",
        "snd_rick_cute": "That's Real Cute",
        "snd_rick_dance": "Let's Dance",
        "snd_rick_devil": "Devil You Know",
        "snd_rick_drink": "I Need A Drink",
        "snd_rick_errand": "Quick 20-Minute Errand",
        "snd_rick_gaf": "I Don't Give A F***",
        "snd_rick_girls": "Girls, Girls, Calm Down",
        "snd_rick_goback": "Go Back In Time",
        "snd_rick_hands": "Keep Your Hands Off",
        "snd_rick_icecream": "Ice Cream Stop",
        "snd_rick_imhaving": "I'm Having An Episode",
        "snd_rick_know": "I Know Everything",
        "snd_rick_makeout": "Make Out Session",
        "snd_rick_meet": "Meet The New Boss",
        "snd_rick_months": "Nine More Seasons Morty",
        "snd_rick_more": "There's Plenty More",
        "snd_rick_osmf": "Oh Sh**, Motherf***",
        "snd_rick_os": "Operating System Reboot",
        "snd_rick_oldsong": "Play That Old Song",
        "snd_rick_operation": "Operation Phoenix",
        "snd_rick_page": "Turn The Page",
        "snd_rick_party": "Party Time",
        "snd_rick_people": "I Hate People",
        "snd_rick_player": "Don't Hate The Player",
        "snd_rick_rickless": "No Rick Is An Island",
        "snd_rick_rubber": "Rubber Shoes",
        "snd_rick_satisfaction": "Satisfaction Guaranteed",
        "snd_rick_shady": "Shady Dealings",
        "snd_rick_sleep": "Sleep Is For The Weak",
        "snd_rick_sucks": "This Sucks",
        "snd_rick_sweetie": "Hey Sweetie",
        "snd_rick_thankyou": "Thank You Very Much",
        "snd_rick_whiskey": "Pass The Whiskey",
        "snd_rick_yorn": "Yes Or No",
        "snd_rick_riggity": "Riggity Riggity Wrecked Son!",
        "snd_rick_rickytiffy": "Ricky Tiffy Taffy Bitch!",
        "snd_rick_wubba": "Wubba Lubba Dub Dub!",
        "snd_rick_rickmorty": "Rick And Morty 100 Years!",
    }

    manifest = []

    for idx, (arr_name, label) in enumerate(soundboard_entries, 1):
        clean_ident = arr_name.replace("snd_rick_", "").replace("snd_", "")
        fname = f"{idx:02d}_{label}_{clean_ident}.wav"
        data = None
        if arr_name in rick_arrays:
            data = rick_arrays[arr_name]
        elif arr_name in portal_arrays:
            data = portal_arrays[arr_name]
        
        if data:
            out_p = os.path.join(RICK_DIR, fname)
            save_wav(out_p, data)
            pk = max(abs(b - 128) for b in data)
            dur = len(data) / 11025.0
            desc = friendly_names.get(arr_name, clean_ident)
            manifest.append({
                "index": idx,
                "label": label,
                "file": fname,
                "rel_path": f"03_rick_soundboard/{fname}",
                "desc": desc,
                "duration": dur,
                "peak": pk
            })
            if idx <= 10 or idx >= len(soundboard_entries) - 3:
                print(f"  [{idx:02d}] {label:4s} | {fname:32s} | {dur:4.2f}s | Peak: {pk}/127 | {desc}")
            elif idx == 11:
                print("  ... (all 44 quotes exported) ...")

    print("\n" + "=" * 70)
    print(f"SUCCESS: Exported all {len(portal_map) + len(theme_map) + len(soundboard_entries)} normalized tracks to:")
    print(f"  -> {os.path.abspath(TARGET_DIR)}")
    print("=" * 70)

if __name__ == "__main__":
    main()

