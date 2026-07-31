#
#  Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#

# 1. Pops up a file picker so you can select one or multiple .m64 files.
# 2. Loops through each file and checks if an old parity check file already exists.
#    - If it does, it pauses and asks you: "Compare or Overwrite?"
# 3. Runs a parity check using mupen (scraping its logs for ParityChecker sample data)
# 4. If you chose to compare, it diffs using git diff to show you where emulation diverged.

import json
import os
import re
import subprocess
import tkinter as tk
from pathlib import Path
from tkinter import filedialog


def main():
    mupen_exe = Path("build/out/mupen64.exe")
    log_dir = Path("build/out/logs")
    log_file = log_dir / "mupen.log"

    log_dir.mkdir(parents=True, exist_ok=True)

    root = tk.Tk()
    root.withdraw()

    file_paths = filedialog.askopenfilenames(
        title="Select .m64 files", filetypes=[("M64 Files", "*.m64")]
    )

    if not file_paths:
        print("No files selected. Exiting.")
        return

    parity_pattern = re.compile(
        r"\[ParityChecker\]\s+sample\s+(\d+)\s+->\s+([a-f0-9]+)", re.IGNORECASE
    )

    for file_path in file_paths:
        file_path_obj = Path(file_path)
        filename = file_path_obj.name
        out_json_path = log_dir / f"PARITY_{filename}.json"

        print(f"\n{'=' * 50}\nProcessing: {filename}")

        # Check for existing parity JSON and ask the user what to do
        choice = "o"
        if out_json_path.exists():
            print(f"⚠️  Found existing parity file: {out_json_path.name}")
            while True:
                choice = (
                    input(
                        "   Do you want to (c)ompare against the old data or (o)verwrite it? [c/o]: "
                    )
                    .strip()
                    .lower()
                )
                if choice in ["c", "o"]:
                    break
                print(
                    "   Invalid choice. Please enter 'c' to compare or 'o' to overwrite."
                )

        # Delete the mupen log file BEFORE each run
        if log_file.exists():
            try:
                log_file.unlink()
            except Exception as e:
                print(f"Failed to delete {log_file}: {e}")

        # Execute mupen64
        cmd = [
            str(mupen_exe),
            "-g",
            str(file_path_obj),
            "-m64",
            str(file_path_obj),
            "--parity-check",
        ]
        print(f"Executing: {' '.join(cmd)}")
        subprocess.run(cmd)

        # Parse the new log
        new_parity_data = []
        if log_file.exists():
            with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    match = parity_pattern.search(line)
                    if match:
                        new_parity_data.append(
                            {"sample": match.group(1), "checksum": match.group(2)}
                        )
        else:
            print(f"Warning: {log_file} was not generated for {filename}")

        if choice == "c":
            # Write to a temporary file for the diff
            temp_json_path = out_json_path.with_suffix(".tmp.json")
            with open(temp_json_path, "w", encoding="utf-8") as f:
                json.dump(new_parity_data, f, indent=4)

            print(f"\n--- Comparing Parity for {filename} ---")

            # Run git diff between the old file and the temp new file
            diff_cmd = [
                "git",
                "--no-pager",
                "diff",
                "--no-index",
                "--color=always",
                str(out_json_path),
                str(temp_json_path),
            ]

            # Subprocess will print the diff directly to the console
            result = subprocess.run(diff_cmd)

            # git diff returns exit code 0 if there are no differences
            if result.returncode == 0:
                print(
                    "\n✅ 🎉 SUCCESS: The new parity data matches the existing file perfectly!"
                )
            else:
                print("\n❌ Divergence detected! See the diff above.")

            # Overwrite the old file with the new file
            temp_json_path.replace(out_json_path)
            print(f"Saved new parity data to: {out_json_path}")

        else:
            # Save the newly collected data directly, overwriting the old file
            with open(out_json_path, "w", encoding="utf-8") as f:
                json.dump(new_parity_data, f, indent=4)
            print(f"Saved new parity data to: {out_json_path}")

    print("\nAll files processed successfully.")


if __name__ == "__main__":
    main()
