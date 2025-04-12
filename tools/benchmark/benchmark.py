#
# Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
# 
# SPDX-License-Identifier: GPL-2.0-or-later
# 

# Benchmarks the emulator's performance on the latest and second-latest commit of the currently checked out branch.
# Performs a hard git reset when rolling back to the latest commit.
# Outputs performance results to the output stream and to a JSON file.
# Utilizes the [mupen64plus test ROM](https://github.com/mupen64plus/mupen64plus-rom) for benchmarking.

# Requires Mupen64 to be built in the Views.Win32 directory.

import json
import subprocess

MUPEN_PATH = "../../build/Views.Win32/mupen64-x86-sse2-release.exe"

STANDARD_ARGS = [  '-g', "..\\m64p_test_rom.v64", '-m64', 'test_rom_benchmark.m64' ]

def get_commit_hash():
    result = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True, check=True)
    return result.stdout.strip()

def run_mupen(name, additional_args):
    benchmark_path = f"benchmark_{name}.json"

    # First run is warmup.
    for _ in range(2):
        subprocess.run([MUPEN_PATH, *STANDARD_ARGS, *additional_args, '-b', benchmark_path], timeout=120)

    with open(benchmark_path) as f:
        data = json.load(f)
    return data

def run_benchmark_full(name, additional_args=[]):
    new_hash = get_commit_hash()
    benchmark_new = run_mupen(get_commit_hash(), additional_args)
    
    subprocess.run(['git', 'stash'])
    subprocess.run(['git', 'reset', '--hard', 'HEAD~1'])
    old_hash = get_commit_hash()
    benchmark_old = run_mupen(get_commit_hash(), additional_args)

    subprocess.run(['git', 'reset', '--hard', 'HEAD@{1}'])
    subprocess.run(['git', 'stash', 'pop'])

    new_fps = benchmark_new['fps']
    old_fps = benchmark_old['fps']

    print(f"Benchmark - {name} ({old_hash}) vs {new_hash}")
    delta = new_fps - old_fps
    percentage_change = (delta / old_fps) * 100

    print(f"FPS: {old_fps:.2f} (old) | {old_fps:.2f} (new)")
    print(f"Change: {percentage_change:.2f}%")
    print("------")

def main():
    run_benchmark_full("normal")
    run_benchmark_full("with-dummy-lua", ["-lua", "dummy.lua"])
    

if __name__ == "__main__":
    main()