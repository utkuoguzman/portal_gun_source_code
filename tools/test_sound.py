import sys
import subprocess
import os

PORTAL_DIR = "soundbites/portal_sounds"
ROULETTE_DIR = "soundbites/roullette"
EVIL_MORTY_DIR = "soundbites/evil_morty"
RICK_DIR = "soundbites/rick_soundbites"

FEATURED_SOUNDS = {
    # Stock Portal Gun Sounds
    "1": ("Portal Gun: Power-Up Sound (Startup)", os.path.join(PORTAL_DIR, "snd_powerup.wav"), []),
    "2": ("Portal Gun: Fire Blast 1", os.path.join(PORTAL_DIR, "snd_portal_open1.wav"), []),
    "3": ("Portal Gun: Fire Blast 2", os.path.join(PORTAL_DIR, "snd_portal_open2.wav"), []),
    "4": ("Portal Gun: Portal Close", os.path.join(PORTAL_DIR, "snd_portal_close1.wav"), []),
    "5": ("Rick & Morty: Get Schwifty (Full Song)", os.path.join(PORTAL_DIR, "snd_getschwifty.wav"), []),
    
    # Roulette Game Sounds
    "6": ("Roulette: Spin (Click-click-click)", os.path.join(ROULETTE_DIR, "Click-click-click.m4a"), []),
    "7": ("Roulette: Safe Strike (Clank)", os.path.join(ROULETTE_DIR, "Clank.m4a"), []),
    "8": ("Roulette: Live Boom (Boom)", os.path.join(ROULETTE_DIR, "Boom.m4a"), []),
    "9": ("Roulette: Gloat - Wrecked Son (Isolated)", os.path.join(ROULETTE_DIR, "Wrecked_Son.m4a"), []),
    
    # Evil Morty & Golden Portal
    "10": ("Evil Morty: Theme Song (0:32 - 1:53)", os.path.join(EVIL_MORTY_DIR, "Evil Morty Final Theme.mp3"), ["-ss", "00:00:32", "-t", "81"]),
    
    # Rick Sanchez Soundboard Classics
    "11": ("Rick Voice: Pickle Rick!", os.path.join(RICK_DIR, "Voicy_Pickle Rick!!!.mp3"), []),
    "12": ("Rick Voice: Wubba Lubba Dub Dub!", os.path.join(RICK_DIR, "Voicy_Wubba Lubba Dub Dub (rick and morty).mp3"), []),
    "13": ("Rick Voice: Ricky Tiffy Taffy Bitch!", os.path.join(RICK_DIR, "Voicy_Ricky Tiffy Taffy Bitch.mp3"), []),
    "14": ("Rick Voice: What is my purpose? (Butter)", os.path.join(RICK_DIR, "Voicy_Rick Sanchez Butter.mp3"), []),
    "15": ("Rick Voice: What's up Bitch?", os.path.join(RICK_DIR, "Voicy_Rick Sanchez Btch.mp3"), []),
}

def play_file(title, path, extra=None):
    if extra is None:
        extra = []
    if not os.path.exists(path):
        print(f"\n[ERROR] File not found: {path}")
        return False
    print(f"\n>> Playing: {title} ({path})...")
    cmd = ["ffplay", "-nodisp", "-autoexit"] + extra + [path]
    try:
        res = subprocess.run(cmd, stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
        if res.returncode != 0:
            err = res.stderr.decode('utf-8', errors='ignore')
            print(f"[ERROR] Playback failed (code {res.returncode}):\n{err[:300]}")
            return False
        return True
    except Exception as e:
        print("[ERROR] Exception running ffplay:", e)
        return False

def verify_all_audio():
    print("\n--- RUNNING AUDIO ASSET INTEGRITY SCAN ---")
    all_ok = True
    
    # 1. Portal sounds
    portal_expected = ["snd_powerup.wav", "snd_buckleup.wav", "snd_portal_open1.wav", "snd_portal_open2.wav", "snd_portal_close1.wav", "snd_getschwifty.wav"]
    for f in portal_expected:
        p = os.path.join(PORTAL_DIR, f)
        if not os.path.exists(p) or os.path.getsize(p) == 0:
            print(f"  [MISSING/EMPTY] Portal sound: {p}")
            all_ok = False
        else:
            print(f"  [OK] Portal sound: {f} ({os.path.getsize(p)} bytes)")

    # 2. Roulette files
    roulette_expected = ["Click-click-click.m4a", "Clank.m4a", "Boom.m4a", "Wrecked_Son.m4a"]
    for f in roulette_expected:
        p = os.path.join(ROULETTE_DIR, f)
        if not os.path.exists(p) or os.path.getsize(p) == 0:
            print(f"  [MISSING/EMPTY] Roulette: {p}")
            all_ok = False
        else:
            print(f"  [OK] Roulette: {f} ({os.path.getsize(p)} bytes)")

    # 3. Evil Morty file
    em_path = os.path.join(EVIL_MORTY_DIR, "Evil Morty Final Theme.mp3")
    if not os.path.exists(em_path) or os.path.getsize(em_path) == 0:
        print(f"  [MISSING/EMPTY] Evil Morty: {em_path}")
        all_ok = False
    else:
        print(f"  [OK] Evil Morty: Evil Morty Final Theme.mp3 ({os.path.getsize(em_path)} bytes)")

    # 4. Rick Soundboard files
    rick_files = [f for f in os.listdir(RICK_DIR) if f.endswith('.mp3')]
    print(f"  Checking {len(rick_files)} Rick Soundboard files...")
    for f in rick_files:
        p = os.path.join(RICK_DIR, f)
        sz = os.path.getsize(p)
        if sz == 0:
            print(f"  [EMPTY] {f}")
            all_ok = False
    
    total = len(portal_expected) + len(roulette_expected) + 1 + len(rick_files)
    if all_ok:
        print(f"\n>> SUCCESS: All {total} audio files verified on disk with non-zero size!")
    return all_ok

def list_all_rick_soundbites():
    rick_files = sorted([f for f in os.listdir(RICK_DIR) if f.endswith('.mp3')])
    print("\n" + "=" * 60)
    print(f"  ALL RICK SOUNDBOARD FILES ({len(rick_files)} total)")
    print("=" * 60)
    for idx, f in enumerate(rick_files, 1):
        clean_name = f.replace("Voicy_", "").replace(".mp3", "")
        print(f"  [{idx:2d}] {clean_name}")
    print("=" * 60)
    choice = input("\nEnter number to play (or press Enter to return): ").strip()
    if choice.isdigit():
        idx = int(choice)
        if 1 <= idx <= len(rick_files):
            target = rick_files[idx - 1]
            play_file(target, os.path.join(RICK_DIR, target))

def main():
    if len(sys.argv) > 1:
        arg = sys.argv[1].lower()
        if arg == "--verify" or arg == "-v":
            verify_all_audio()
            return
        if arg == "l" or arg == "--list":
            list_all_rick_soundbites()
            return
        if arg in FEATURED_SOUNDS:
            t, p, e = FEATURED_SOUNDS[arg]
            play_file(t, p, e)
            return

    print("=" * 65)
    print("      RICK & MORTY PORTAL GUN - AUDIO VERIFICATION PLAYER")
    print("=" * 65)
    print("  --- Portal Gun Audio ---")
    for k in ["1", "2", "3", "4", "5"]:
        print(f"  [{k:2s}] {FEATURED_SOUNDS[k][0]}")
    print("\n  --- Mini-Games & Special Dimensions ---")
    for k in ["6", "7", "8", "9", "10"]:
        print(f"  [{k:2s}] {FEATURED_SOUNDS[k][0]}")
    print("\n  --- Rick Sanchez Soundboard ('R' Dimensions) ---")
    for k in ["11", "12", "13", "14", "15"]:
        print(f"  [{k:2s}] {FEATURED_SOUNDS[k][0]}")
    print("=" * 65)
    print("  [v ] Verify all files on disk")
    print("  [l ] List and play ANY of the 44 Rick voice lines")
    print("  [q ] Quit")
    print("=" * 65)

    while True:
        choice = input("\nSelect a sound to test (1-15, v, l, q): ").strip().lower()
        if choice == 'q':
            break
        elif choice == 'v':
            verify_all_audio()
        elif choice == 'l':
            list_all_rick_soundbites()
        elif choice in FEATURED_SOUNDS:
            t, p, e = FEATURED_SOUNDS[choice]
            play_file(t, p, e)
        else:
            print(f"Unknown choice: {choice}")

if __name__ == "__main__":
    main()
