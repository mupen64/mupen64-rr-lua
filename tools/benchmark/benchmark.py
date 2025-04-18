#
# Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
# 
# SPDX-License-Identifier: GPL-2.0-or-later
# 

# Benchmarks the emulator's performance on the specified commits and compares them.
# Modifies the Mupen config.ini as follows:
# - Sets the "Keep working directory" option to true.
# - Sets the plugins to the dummy plugin suite in ./tools/plugins. 
# Utilizes the [mupen64plus test ROM](https://github.com/mupen64plus/mupen64plus-rom) for benchmarking.
# Requires Mupen64 to be built in the Views.Win32 directory.

import json
import pathlib
import subprocess
import configparser
import traceback

MUPEN_PATH = "../../build/Views.Win32/mupen64-x86-sse2-release.exe"
CONFIG_INI_PATH = "../../build/Views.Win32/config.ini"
STANDARD_ARGS = [  '-g', "..\\roms\\m64p_test_rom.v64", '-m64', 'test_rom_benchmark.m64' ]
FPS_PERCENTAGE_EPSILON = 1
RUN_COUNT = 3

# If left empty, HEAD~1 will be used.
old_commit_hash = ""

# If left empty, HEAD will be used.
new_commit_hash = ""

def fill_commit_hashes():
    global old_commit_hash, new_commit_hash
    if not old_commit_hash:
        old_commit_hash = subprocess.run(['git', 'rev-parse', '--short', 'HEAD^'], capture_output=True, text=True, check=True).stdout.strip()
    if not new_commit_hash:
        new_commit_hash = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True, check=True).stdout.strip()

def run_mupen(name, additional_args):
    benchmark_path = f"benchmark_{name}.json"

    args = [MUPEN_PATH, *STANDARD_ARGS, *additional_args, '-b', benchmark_path]

    print(f"Running {' '.join(args)}")
    
    # Warmup
    subprocess.run(args, timeout=120)

    fps_sum = 0

    for _ in range(RUN_COUNT):
        subprocess.run(args, timeout=120)
        with open(benchmark_path) as f:
            data = json.load(f)
            fps_sum += data['fps']
    
    print(fps_sum)
    fps_avg = fps_sum / RUN_COUNT
    
    return { 'fps': fps_avg } 

def run_benchmark_full(name, additional_args=[]):
    subprocess.run(['git', 'stash', 'push', '-u', '-m', 'benchmark'], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)

    benchmark_new = {}
    benchmark_old = {}

    try:
        subprocess.run(['git', 'reset', '--hard', new_commit_hash], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        benchmark_new = run_mupen(new_commit_hash, additional_args)

        subprocess.run(['git', 'reset', '--hard', old_commit_hash], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        benchmark_old = run_mupen(old_commit_hash, additional_args)

        subprocess.run(['git', 'reset', '--hard', new_commit_hash], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
    except Exception:
        traceback.print_exc()
        print("Error during benchmark. Stash will be popped.")
        subprocess.run(['git', 'stash', 'pop'])
        return
    
    subprocess.run(['git', 'stash', 'pop'], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)

    new_fps = benchmark_new['fps']
    old_fps = benchmark_old['fps']
    delta = new_fps - old_fps
    percentage_change = (delta / old_fps) * 100
    within_margin_of_error = abs(percentage_change) < FPS_PERCENTAGE_EPSILON

    print(f"Benchmark - {name} ({old_commit_hash}) vs {new_commit_hash}")
    print(f"FPS: {old_fps:.2f} (old) | {new_fps:.2f} (new)")
    print(f"Change: {percentage_change:.2f}%")
    if within_margin_of_error:
        print("Within margin of error.")
    else:
        print(percentage_change > 0 and "IMPROVEMENT" or "REGRESSION")
    print("------")

def create_config():
    '''
    Creates a config file with some default values filled in, while deleting any existing config.
    '''

    pathlib.Path.unlink(pathlib.Path(CONFIG_INI_PATH), missing_ok=True)

    config = configparser.ConfigParser()
    config.read(CONFIG_INI_PATH)
    
    config.add_section("config")
    config.set("config", "silent_mode", "1")
    config.set("config", "keep_default_working_directory", "1")
    config.set("config", "selected_video_plugin", "../plugins/NoVideo-x86.dll")
    config.set("config", "selected_audio_plugin", "../plugins/NoAudio-x86.dll")
    config.set("config", "selected_input_plugin", "../plugins/NoInput-x86.dll")
    config.set("config", "selected_rsp_plugin", "../plugins/NoRSP-x86.dll")

    with open(CONFIG_INI_PATH, 'w+') as configfile:
        config.write(configfile)

def main():
    create_config()
    fill_commit_hashes()

    print(f"Running benchmarks for {old_commit_hash} vs {new_commit_hash}...")
    run_benchmark_full("normal")
    run_benchmark_full("with-dummy-lua", ["-lua", "dummy.lua"])
    
    # TODO: Add more benchmarks here.  
    

if __name__ == "__main__":
    main()