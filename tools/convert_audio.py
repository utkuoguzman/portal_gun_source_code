import os
import sys
import subprocess

# This script converts .mp3 and .m4a files in soundbites/ to C++ arrays
# Requirements: ffmpeg installed and in your system PATH

def main():
    source_dir = "soundbites"
    out_h = "include/audio_assets.h"
    out_cpp = "src/modules/audio_assets.cpp"

    if not os.path.exists(source_dir):
        print(f"Error: {source_dir} directory not found. Create it and put your sound files there.")
        return

    # Files to process and their corresponding C array names
    files_to_process = {
        "PowerUpSound.mp3": "snd_powerup",
        "BuckleUpBitches.m4a": "snd_buckleup",
        "PortalOpenSound1.m4a": "snd_portal_open1",
        "PortalOpenSound2.m4a": "snd_portal_open2",
        "PortalCloseSound1.m4a": "snd_portal_close1",
        "PortalCloseSound2.m4a": "snd_portal_close2",
        "GetSchwiftySong.mp3": "snd_getschwifty",
        "PowerDownSound.mp3": "snd_powerdown"
    }

    header_content = """#pragma once
#include <stdint.h>
#include <stddef.h>

struct SubtitleLine {
    uint32_t startTimeMs;
    uint32_t endTimeMs;
    const char* text;
};

"""
    cpp_content = """#include "audio_assets.h"
// GENERATED FILE - DO NOT EDIT MANUALLY

"""

    os.makedirs(os.path.dirname(out_cpp), exist_ok=True)
    os.makedirs(os.path.dirname(out_h), exist_ok=True)

    for filename, array_name in files_to_process.items():
        in_path = os.path.join(source_dir, filename)
        
        header_content += f"extern const uint8_t {array_name}[];\n"
        header_content += f"extern const size_t {array_name}_size;\n\n"

        if not os.path.exists(in_path):
            print(f"Warning: {in_path} not found. Creating dummy empty array.")
            cpp_content += f"const uint8_t {array_name}[] = {{0}};\n"
            cpp_content += f"const size_t {array_name}_size = 0;\n\n"
            
            if array_name == "snd_getschwifty":
                for band_name in ["low", "mid", "high"]:
                    arr_name = f"env_getschwifty_{band_name}"
                    header_content += f"extern const uint8_t {arr_name}[];\n"
                    header_content += f"extern const size_t {arr_name}_size;\n\n"
                    cpp_content += f"const uint8_t {arr_name}[] = {{0}};\n"
                    cpp_content += f"const size_t {arr_name}_size = 0;\n\n"
            continue

        print(f"Processing {in_path} to {array_name}...")
        
        # We extract raw 8-bit unsigned PCM at 22050 Hz
        # -f u8 : raw unsigned 8-bit
        # -ar 11025 : 11.025 kHz to save flash space!
        # -ac 1 : mono
        # Prioritize local ffmpeg.exe if we downloaded it
        ffmpeg_bin = "ffmpeg"
        if os.path.exists("tools/ffmpeg.exe"):
            ffmpeg_bin = "tools/ffmpeg.exe"
            
        cmd = [
            ffmpeg_bin, "-y", "-i", in_path,
            "-f", "u8", "-ar", "11025", "-ac", "1", "temp.raw"
        ]
        
        try:
            # We have the exact path now, so shell=True is not needed (and breaks list args on Windows)
            subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError as e:
            print(f"ffmpeg failed to process {in_path}. Is the audio file corrupted?")
            sys.exit(1)
        except FileNotFoundError:
            # Fallback for Windows if winget path is completely lost
            import shutil
            ffmpeg_path = shutil.which("ffmpeg")
            if not ffmpeg_path:
                local_app_data = os.environ.get('LOCALAPPDATA', '')
                fallback = os.path.join(local_app_data, "Microsoft", "WindowsApps", "ffmpeg.exe")
                if os.path.exists(fallback):
                    cmd[0] = fallback
                    try:
                        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                        continue # Success!
                    except Exception as e:
                        pass
            
            print(f"Failed to find ffmpeg! Windows PATH is being stubborn.")
            print(f"Please run this command in your terminal first to temporarily fix it:")
            print(f"$env:Path = [System.Environment]::GetEnvironmentVariable('Path','Machine') + ';' + [System.Environment]::GetEnvironmentVariable('Path','User')")
            sys.exit(1)
        except Exception as e:
            print(f"Failed to run ffmpeg on {in_path}. Error: {e}")
            sys.exit(1)

        with open("temp.raw", "rb") as f:
            raw_data = f.read()
            
        cpp_content += f"const size_t {array_name}_size = {len(raw_data)};\n"
        cpp_content += f"const uint8_t {array_name}[] = {{\n"
        
        # Write bytes in hex
        for i in range(0, len(raw_data), 16):
            chunk = raw_data[i:i+16]
            cpp_content += "    " + ", ".join([f"0x{b:02x}" for b in chunk]) + ",\n"
        cpp_content += "};\n\n"

        # Generate envelopes for Party Mode
        if array_name == "snd_getschwifty":
            print("Generating 3-band amplitude envelopes for Party Mode...")
            bands = {
                "low": "lowpass=f=250",
                "mid": "bandpass=f=1125:width_type=h:w=1750", # roughly 250 to 2000
                "high": "highpass=f=2000"
            }
            
            for band_name, filter_str in bands.items():
                cmd_band = [
                    ffmpeg_bin, "-y", "-i", in_path,
                    "-af", filter_str,
                    "-f", "u8", "-ar", "11025", "-ac", "1", f"temp_{band_name}.raw"
                ]
                try:
                    subprocess.run(cmd_band, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                except Exception as e:
                    print(f"Failed to generate {band_name} band. Error: {e}")
                    continue
                
                with open(f"temp_{band_name}.raw", "rb") as f:
                    band_raw = f.read()
                
                samples_per_chunk = 1102 # 100ms chunks at 11025Hz
                env_data = []
                for i in range(0, len(band_raw), samples_per_chunk):
                    chunk = band_raw[i:i+samples_per_chunk]
                    if len(chunk) == 0: continue
                    amp = sum(abs(b - 128) for b in chunk) / len(chunk)
                    
                    # Boost factors to make them pop
                    if band_name == "low":
                        env_val = min(255, int(amp * 2.5))
                    elif band_name == "mid":
                        env_val = min(255, int(amp * 3.5))
                    else:
                        env_val = min(255, int(amp * 6.5)) # Massive boost for highs
                    env_data.append(env_val)
                
                os.remove(f"temp_{band_name}.raw")

                arr_name = f"env_getschwifty_{band_name}"
                header_content += f"extern const uint8_t {arr_name}[];\n"
                header_content += f"extern const size_t {arr_name}_size;\n\n"

                cpp_content += f"const size_t {arr_name}_size = {len(env_data)};\n"
                cpp_content += f"const uint8_t {arr_name}[] = {{\n"
                for i in range(0, len(env_data), 16):
                    chunk = env_data[i:i+16]
                    cpp_content += "    " + ", ".join([f"0x{b:02x}" for b in chunk]) + ",\n"
                cpp_content += "};\n\n"
            
            # Parse SRT if it exists
            srt_path = os.path.join(source_dir, "GetSchwiftySongLyrics.srt")
            
            header_content += "extern const SubtitleLine getschwifty_lyrics[];\n"
            header_content += "extern const size_t getschwifty_lyrics_count;\n\n"
            
            if os.path.exists(srt_path):
                print("Found SRT lyrics! Parsing for Party Mode...")
                import re
                with open(srt_path, "r", encoding="utf-8") as srt_f:
                    srt_data = srt_f.read()
                
                # regex to match: 00:00:01,000 --> 00:00:04,500\nText...
                blocks = re.split(r'\n\s*\n', srt_data.strip())
                cpp_content += f"const SubtitleLine getschwifty_lyrics[] = {{\n"
                count = 0
                for block in blocks:
                    lines = block.strip().split('\n')
                    if len(lines) >= 3:
                        time_match = re.match(r'(\d{2}):(\d{2}):(\d{2}),(\d{3})\s*-->\s*(\d{2}):(\d{2}):(\d{2}),(\d{3})', lines[1])
                        if time_match:
                            h1, m1, s1, ms1, h2, m2, s2, ms2 = map(int, time_match.groups())
                            start_ms = (h1 * 3600000) + (m1 * 60000) + (s1 * 1000) + ms1
                            end_ms = (h2 * 3600000) + (m2 * 60000) + (s2 * 1000) + ms2
                            text = " ".join(lines[2:]).replace('"', '\\"').replace('\n', ' ')
                            # strip basic html tags
                            text = re.sub(r'<[^>]+>', '', text).upper()
                            cpp_content += f'    {{{start_ms}, {end_ms}, "{text} "}},\n'
                            count += 1
                cpp_content += f"}};\nconst size_t getschwifty_lyrics_count = {count};\n\n"
            else:
                cpp_content += f"const SubtitleLine getschwifty_lyrics[] = {{}};\nconst size_t getschwifty_lyrics_count = 0;\n\n"

    # Cleanup
    if os.path.exists("temp.raw"):
        os.remove("temp.raw")

    with open(out_h, "w") as f:
        f.write(header_content)
    with open(out_cpp, "w") as f:
        f.write(cpp_content)

    print("Success! Audio assets generated.")

if __name__ == "__main__":
    main()
