Import("env")
import os
import subprocess

def merge_bin_action(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    bootloader = os.path.join(build_dir, "bootloader.bin")
    partitions = os.path.join(build_dir, "partitions.bin")
    firmware = os.path.join(build_dir, "firmware.bin")
    merged = os.path.join(build_dir, "firmware.merged.bin")
    
    esptool_path = os.path.expanduser("~/.platformio/packages/tool-esptoolpy/esptool.py")
    python_exe = env.subst("$PYTHONEXE")
    
    cmd = [
        python_exe,
        esptool_path,
        "--chip", "esp32",
        "merge_bin",
        "-o", merged,
        "--flash_size", "16MB",
        "0x1000", bootloader,
        "0x8000", partitions,
        "0x10000", firmware
    ]
    print(f"Merging binary for Wokwi into {merged}...")
    subprocess.run(cmd, check=True)
    print("Merged successfully for Wokwi!")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin_action)

