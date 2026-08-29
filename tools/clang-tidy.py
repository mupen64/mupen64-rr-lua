#
#  Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#

import argparse
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

CLANG_TIDY = "clang-tidy"
BUILD_DIR = Path("build")


def project_root() -> Path:
    try:
        out = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            check=True,
        )
        return Path(out.stdout.strip())
    except (subprocess.CalledProcessError, FileNotFoundError):
        return Path.cwd()


def load_compile_commands(db_path: Path, source_root: Path) -> list[dict]:
    entries = json.loads(db_path.read_text(encoding="utf-8"))
    filtered = []
    for entry in entries:
        file_path = Path(entry["file"])
        if not file_path.is_absolute():
            file_path = Path(entry["directory"]) / file_path
        file_path = file_path.resolve()
        if source_root in file_path.parents:
            filtered.append(
                {
                    "file": str(file_path),
                    "directory": entry["directory"],
                    "command": entry["command"],
                }
            )
    return filtered


def write_filtered_db(entries: list[dict], db_path: Path) -> None:
    db_path.write_text(json.dumps(entries, indent=2), encoding="utf-8")


WARNING_RE = re.compile(r"^[^:\s]+\[?[^\]]*\]?:\d+:(?:\d+:)? warning:", re.MULTILINE)


def run_clang_tidy(
    entry: dict, build_dir: Path, check_only: bool
) -> tuple[str, int, str]:
    cmd = [
        CLANG_TIDY,
        f"--p={build_dir}",
        "--format-style=file",
        entry["file"],
    ]
    if not check_only:
        cmd[1:1] = ["--fix", "--fix-errors"]

    proc = subprocess.run(
        cmd,
        cwd=entry["directory"],
        capture_output=True,
        text=True,
    )
    output = proc.stdout + proc.stderr
    return entry["file"], proc.returncode, output


def main() -> int:
    parser = argparse.ArgumentParser(description="Run clang-tidy over src/ files.")
    parser.add_argument(
        "--check",
        action="store_true",
        help="only check for warnings (no fixes applied); exit 1 if any warnings are found",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=os.cpu_count() or 1,
        help="number of clang-tidy processes to run in parallel (default: number of CPUs)",
    )
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be >= 1")

    root = project_root()
    build_dir = (root / BUILD_DIR).resolve()
    db_path = build_dir / "compile_commands.json"
    source_root = root / "src"

    print("Make sure the project is freshly configured")

    if not db_path.is_file():
        print(
            f"error: {db_path} not found. Configure the project first.", file=sys.stderr
        )
        return 1

    try:
        proc = subprocess.run(
            [CLANG_TIDY, "--version"],
            capture_output=True,
            text=True,
            check=True,
        )
        version_line = proc.stdout.splitlines()[0]
    except (subprocess.CalledProcessError, FileNotFoundError):
        print(f"error: '{CLANG_TIDY}' not found or not runnable.", file=sys.stderr)
        return 1

    entries = load_compile_commands(db_path, source_root)
    if not entries:
        print(
            f"error: no compile commands for files under {source_root}.",
            file=sys.stderr,
        )
        return 1

    print(f"{version_line}")
    print(
        f"Checking {len(entries)} files under src/ ..."
        if args.check
        else f"Fixing {len(entries)} files under src/ ..."
    )

    filtered_db = build_dir / "compile_commands.filtered.json"
    write_filtered_db(entries, filtered_db)

    failures: list[tuple[str, int, str]] = []
    warnings_found: list[str] = []
    try:
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futures = {
                pool.submit(run_clang_tidy, entry, build_dir, args.check): entry["file"]
                for entry in entries
            }
            for future in as_completed(futures):
                file_path, returncode, output = future.result()
                has_warnings = bool(WARNING_RE.search(output))
                if args.check:
                    status = "warnings" if has_warnings else "ok"
                else:
                    status = "ok" if returncode == 0 else f"exit {returncode}"
                print(f"[{status}] {file_path}")
                if returncode != 0:
                    failures.append((file_path, returncode, output))
                elif args.check and has_warnings:
                    warnings_found.append(file_path)
                    print(output)
                elif output.strip():
                    print(output)
    finally:
        filtered_db.unlink(missing_ok=True)

    failures.sort()
    warnings_found.sort()

    if failures:
        print(f"\n{len(failures)} file(s) failed:")
        for file_path, returncode, output in failures:
            print(f"\n=== {file_path} (exit {returncode}) ===")
            print(output)
        return 1

    if args.check and warnings_found:
        print(
            f"\n{len(warnings_found)} file(s) with clang-tidy warnings:"
        )
        for file_path in warnings_found:
            print(file_path)
        return 1

    print("\nDone.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
