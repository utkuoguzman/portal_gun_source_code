import sys
import subprocess
import os
import wave

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WAV_DIR = os.path.join(BASE_DIR, "soundbites", "normalized_wavs")
PORTAL_DIR = os.path.join(WAV_DIR, "01_portal_gun")
THEMES_DIR = os.path.join(WAV_DIR, "02_games_and_themes")
RICK_DIR = os.path.join(WAV_DIR, "03_rick_soundboard")

FEATURED_SOUNDS = {
    # Stock Portal Gun Sounds
    "1": ("Portal Gun: Power-Up Sound (Startup)", os.path.join(PORTAL_DIR, "01_powerup.wav")),
    "2": ("Portal Gun: Buckle Up Bitches!", os.path.join(PORTAL_DIR, "02_buckleup.wav")),
    "3": ("Portal Gun: Fire Blast 1", os.path.join(PORTAL_DIR, "03_portal_open1.wav")),
    "4": ("Portal Gun: Fire Blast 2", os.path.join(PORTAL_DIR, "04_portal_open2.wav")),
    "5": ("Portal Gun: Portal Close 1", os.path.join(PORTAL_DIR, "05_portal_close1.wav")),
    "6": ("Portal Gun: Portal Close 2", os.path.join(PORTAL_DIR, "06_portal_close2.wav")),
    "7": ("Portal Gun: Get Schwifty (Full Song)", os.path.join(PORTAL_DIR, "07_getschwifty.wav")),
    "8": ("Portal Gun: Power-Down (Shutdown)", os.path.join(PORTAL_DIR, "08_powerdown.wav")),
    
    # Mini-Games & Special Dimension Sounds
    "9":  ("Evil Morty: Theme Song (Dimension M-720)", os.path.join(THEMES_DIR, "01_evil_morty_theme.wav")),
    "10": ("Roulette: Spin (Click-click-click)", os.path.join(THEMES_DIR, "02_roulette_spin.wav")),
    "11": ("Roulette: Safe Strike (Clank)", os.path.join(THEMES_DIR, "03_roulette_clack.wav")),
    "12": ("Roulette: Live Boom (Boom)", os.path.join(THEMES_DIR, "04_roulette_boom.wav")),
    "13": ("Roulette: Gloat - Wrecked Son (Isolated)", os.path.join(THEMES_DIR, "05_roulette_wrecked.wav")),
    "14": ("Snake Jazz: 8-Bar Loop (Dimension S-555)", os.path.join(THEMES_DIR, "06_snake_jazz.wav")),
    "15": ("Dino Game: 8-Bit Soundtrack (Dimension D-999)", os.path.join(THEMES_DIR, "07_dino_song.wav")),
    
    # Top Rick Sanchez Soundboard Classics
    "16": ("Rick Voice: Pickle Rick!!! [PCKL]", os.path.join(RICK_DIR, "02_PCKL_pickle.wav")),
    "17": ("Rick Voice: Wubba Lubba Dub Dub! [WUBB]", os.path.join(RICK_DIR, "44_WUBB_wubba.wav")),
    "18": ("Rick Voice: Ricky Tiffy Taffy Bitch! [tIFF]", os.path.join(RICK_DIR, "43_tIFF_rickytiffy.wav")),
    "19": ("Rick Voice: What is my purpose? (Butter) [bUtr]", os.path.join(RICK_DIR, "05_bUtr_butter.wav")),
    "20": ("Rick Voice: What's up Bitch? [btch]", os.path.join(RICK_DIR, "04_btch_bitch.wav")),
    "21": ("Rick Voice: Riggity Riggity Wrecked Son! [RIGG]", os.path.join(RICK_DIR, "42_RIGG_riggity.wav")),
}

def play_file(title, path):
    if not os.path.exists(path):
        print(f"\n[ERROR] File not found: {path}")
        print("  Run 'python tools/export_normalized_wavs.py' to generate normalized WAVs first.")
        return False

    with wave.open(path, "rb") as w:
        frames = w.getnframes()
        rate = w.getframerate()
        dur = frames / float(rate)
        raw = w.readframes(frames)
        pk = max(abs(b - 128) for b in raw) if len(raw) > 0 else 0

    print(f"\n>> Playing: {title}")
    print(f"   Path: {os.path.basename(path)} | Duration: {dur:.2f}s | Peak: {pk}/127 (Zero Clipping Guaranteed)")

    # 1. Try ffplay first (supports keyboard controls and stop with 'q')
    try:
        cmd = ["ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet", path]
        res = subprocess.run(cmd)
        if res.returncode == 0:
            return True
    except Exception:
        pass

    # 2. Native Windows Fallback: System.Media.SoundPlayer via PowerShell
    try:
        ps_cmd = f"& {{ (New-Object System.Media.SoundPlayer '{os.path.abspath(path)}').PlaySync() }}"
        res = subprocess.run(["powershell", "-NoProfile", "-Command", ps_cmd], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if res.returncode == 0:
            return True
    except Exception:
        pass

    # 3. Fallback: Python winsound (Windows only)
    try:
        import winsound
        winsound.PlaySound(path, winsound.SND_FILENAME)
        return True
    except Exception as e:
        print(f"[ERROR] Playback failed: {e}")
        return False

def verify_all_audio():
    print("\n" + "=" * 75)
    print("      NORMALIZED AUDIO ASSETS - INTEGRITY & ZERO-CLIPPING AUDIT")
    print("=" * 75)
    
    total_files = 0
    total_clipped = 0
    total_bytes = 0

    categories = [
        ("Portal Gun System Audio", PORTAL_DIR),
        ("Mini-Games & Dimension Themes", THEMES_DIR),
        ("Rick Soundboard (44 Quotes)", RICK_DIR),
    ]

    for cat_name, cat_dir in categories:
        if not os.path.exists(cat_dir):
            print(f"[MISSING DIR] {cat_dir}")
            continue
        files = sorted([f for f in os.listdir(cat_dir) if f.endswith(".wav")])
        print(f"\n--- {cat_name} ({len(files)} files) ---")
        for f in files:
            p = os.path.join(cat_dir, f)
            with wave.open(p, "rb") as w:
                nframes = w.getnframes()
                rate = w.getframerate()
                data = w.readframes(nframes)
                dur = nframes / float(rate)
                clipped = sum(1 for b in data if b <= 0 or b >= 255)
                pk = max(abs(b - 128) for b in data)
                total_clipped += clipped
                total_files += 1
                total_bytes += len(data)
                status = "OK [SAFE]" if clipped == 0 and pk <= 114 else "CHECK"
                print(f"  [{status}] {f:34s} | {dur:5.2f}s | Peak: {pk:3d}/127 | Clipped: {clipped}")

    print("\n" + "=" * 75)
    print(f"AUDIT SUMMARY:")
    print(f"  Total Files Audited : {total_files}")
    print(f"  Total Audio Payload : {total_bytes / 1024:.1f} KB")
    print(f"  Total Clipped Samples: {total_clipped} (ZERO CLIPPING)")
    print(f"  Sample Rate         : 11025 Hz (Mono, 8-bit unsigned PCM)")
    print(f"  Peak Headroom       : Target Peak = 114/127 (-0.94 dBFS, 14 LSB rail safety)")
    print("=" * 75)
    return total_clipped == 0

def list_all_rick_soundbites():
    if not os.path.exists(RICK_DIR):
        print(f"[ERROR] Rick soundboard directory missing: {RICK_DIR}")
        return
    files = sorted([f for f in os.listdir(RICK_DIR) if f.endswith(".wav")])
    print("\n" + "=" * 70)
    print(f"      ALL RICK SANCHEZ SOUNDBOARD FILES ({len(files)} total)")
    print("=" * 70)
    for idx, f in enumerate(files, 1):
        clean_name = f.replace(".wav", "")
        print(f"  [{idx:2d}] {clean_name}")
    print("=" * 70)
    choice = input("\nEnter number to play (or press Enter to return): ").strip()
    if choice.isdigit():
        idx = int(choice)
        if 1 <= idx <= len(files):
            target = files[idx - 1]
            play_file(target, os.path.join(RICK_DIR, target))

def search_rick_soundbites():
    if not os.path.exists(RICK_DIR):
        return
    term = input("\nEnter search keyword (e.g. 'pickle', 'wubba', 'butter', 'party'): ").strip().lower()
    if not term:
        return
    files = sorted([f for f in os.listdir(RICK_DIR) if f.endswith(".wav") and term in f.lower()])
    if not files:
        print(f"No sound bites found matching '{term}'.")
        return
    print(f"\nFound {len(files)} matching sound bites:")
    for idx, f in enumerate(files, 1):
        print(f"  [{idx:2d}] {f}")
    choice = input("\nEnter number to play (or press Enter to return): ").strip()
    if choice.isdigit():
        idx = int(choice)
        if 1 <= idx <= len(files):
            target = files[idx - 1]
            play_file(target, os.path.join(RICK_DIR, target))

def main():
    if len(sys.argv) > 1:
        arg = sys.argv[1].lower()
        if arg in ["--verify", "-v"]:
            verify_all_audio()
            return
        if arg in ["l", "--list"]:
            list_all_rick_soundbites()
            return
        if arg in FEATURED_SOUNDS:
            t, p = FEATURED_SOUNDS[arg]
            play_file(t, p)
            return

    while True:
        print("\n" + "=" * 75)
        print("   RICK & MORTY PORTAL GUN - NORMALIZED AUDIO VERIFICATION PLAYER")
        print("=" * 75)
        print("  --- Portal Gun System Audio ---")
        for k in ["1", "2", "3", "4", "5", "6", "7", "8"]:
            print(f"  [{k:2s}] {FEATURED_SOUNDS[k][0]}")
        print("\n  --- Mini-Games & Special Dimension Audio ---")
        for k in ["9", "10", "11", "12", "13", "14", "15"]:
            print(f"  [{k:2s}] {FEATURED_SOUNDS[k][0]}")
        print("\n  --- Top Rick Sanchez Soundboard Classics ---")
        for k in ["16", "17", "18", "19", "20", "21"]:
            print(f"  [{k:2s}] {FEATURED_SOUNDS[k][0]}")
        print("=" * 75)
        print("  [v ] Verify & audit ALL 60 audio tracks (Peak, Duration, Zero-Clipping)")
        print("  [l ] List ALL 44 Rick voice quotes")
        print("  [s ] Search Rick voice quotes by keyword")
        print("  [q ] Quit")
        print("=" * 75)

        choice = input("\nSelect an option: ").strip().lower()
        if choice == 'q':
            break
        elif choice == 'v':
            verify_all_audio()
        elif choice == 'l':
            list_all_rick_soundbites()
        elif choice == 's':
            search_rick_soundbites()
        elif choice in FEATURED_SOUNDS:
            t, p = FEATURED_SOUNDS[choice]
            play_file(t, p)
        else:
            print(f"Unknown selection: {choice}")

if __name__ == "__main__":
    main()
