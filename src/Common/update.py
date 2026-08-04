#!/usr/bin/env python3
#
#  Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#

"""
TUI auto-updater for Mupen64.

Downloads a channel artifact from GitHub and performs an overwriting merge
into the directory this script lives in (i.e. the Mupen64 installation).

Usage:
    python update.py [--channel {stable-w32,stable-w64,nightly-w32,nightly-w64}]

When --channel is omitted, the user is prompted to choose one. When mupen
invokes the updater it passes the channel that fits the running build, and
the channel prompt is skipped.
"""

import argparse
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

CHANNEL_URLS = {
    "stable-w32": "https://github.com/mupen64/repack-stable-w32/archive/refs/heads/main.zip",
    "stable-w64": "https://github.com/mupen64/repack-stable-w64/archive/refs/heads/main.zip",
    "nightly-w32": "https://github.com/mupen64/repack-nightly-w32/archive/refs/heads/main.zip",
    "nightly-w64": "https://github.com/mupen64/repack-nightly-w64/archive/refs/heads/main.zip",
}

MUPEN_PROCESS_NAME = "mupen64.exe"
DOWNLOAD_CHUNK_SIZE = 256 * 1024


def read_line(prompt: str = "") -> str:
    try:
        return input(prompt)
    except EOFError:
        print()
        return ""


def ask_yes_no(prompt: str) -> bool:
    while True:
        answer = read_line(f"{prompt} [y/N] ").strip().lower()
        if answer in ("y", "yes"):
            return True
        if answer in ("n", "no", ""):
            return False
        print("Please answer 'y' or 'n'.")


def prompt_channel() -> str:
    names = list(CHANNEL_URLS)
    print("Available channels:")
    for index, name in enumerate(names, 1):
        print(f"  {index}) {name}")
    while True:
        raw = read_line(f"Select a channel [1-{len(names)}]: ").strip()
        if not raw:
            print("Update aborted.")
            raise SystemExit(1)
        if not raw.isdigit():
            print("Please enter a number.")
            continue
        index = int(raw)
        if 1 <= index <= len(names):
            return names[index - 1]
        print(f"Please enter a number between 1 and {len(names)}.")


def format_size(num_bytes: int) -> str:
    return f"{num_bytes / (1024 * 1024):.1f} MiB"


def download(url: str, dest: Path) -> None:
    print(f"Downloading {url}")
    with urllib.request.urlopen(url) as response, open(dest, "wb") as out:
        total = int(response.headers.get("Content-Length") or 0)
        downloaded = 0
        last_percent = -1
        while True:
            chunk = response.read(DOWNLOAD_CHUNK_SIZE)
            if not chunk:
                break
            out.write(chunk)
            downloaded += len(chunk)
            if total > 0:
                percent = downloaded * 100 // total
                if percent != last_percent:
                    print(
                        f"\r  {format_size(downloaded)} / {format_size(total)} ({percent}%)",
                        end="",
                        flush=True,
                    )
                    last_percent = percent
    if total > 0:
        print()
    else:
        print(f"  Downloaded {format_size(downloaded)}.")


def _linux_mupen_pids() -> list[str]:
    try:
        result = subprocess.run(
            ["pgrep", "-x", MUPEN_PROCESS_NAME],
            capture_output=True,
            text=True,
            check=False,
        )
    except FileNotFoundError:
        return []
    if result.returncode != 0:
        return []
    return [pid for pid in result.stdout.split() if pid]


def mupen_is_running() -> bool:
    if sys.platform == "win32":
        try:
            result = subprocess.run(
                ["tasklist", "/FI", f"IMAGENAME eq {MUPEN_PROCESS_NAME}"],
                capture_output=True,
                text=True,
                check=True,
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
        return MUPEN_PROCESS_NAME.lower() in result.stdout.lower()
    return bool(_linux_mupen_pids())


def kill_mupen() -> None:
    if sys.platform == "win32":
        subprocess.run(["taskkill", "/IM", MUPEN_PROCESS_NAME, "/F"], check=False)
        return
    # taskkill is a Windows tool; on Linux signal the Wine-hosted mupen64.exe
    # processes directly.
    pids = _linux_mupen_pids()
    if pids:
        subprocess.run(["kill", "-9", *pids], check=False)


def _wait_for_mupen_exit(timeout: float = 10.0) -> None:
    if sys.platform == "win32":
        return
    import time

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if not mupen_is_running():
            return
        time.sleep(0.2)
    print("Warning: mupen64.exe did not exit in time; continuing anyway.")


def _remove_path(path: Path) -> None:
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path)
    else:
        path.unlink()


def _ensure_directory(path: Path) -> None:
    if path.exists():
        if path.is_dir():
            return
        _remove_path(path)
    parent = path.parent
    if parent != path:
        _ensure_directory(parent)
    path.mkdir()


def extract_merge(zip_path: Path, dest: Path) -> None:
    with zipfile.ZipFile(zip_path) as archive:
        entries = [entry for entry in archive.infolist() if entry.filename]
        if not entries:
            return

        strip_parts = 0
        first_part = Path(entries[0].filename).parts[0]
        if all(Path(entry.filename).parts[0] == first_part for entry in entries):
            strip_parts = 1

        files = [entry for entry in entries if not entry.is_dir()]
        print(f"Extracting {len(files)} file(s) into {dest} ...")

        for index, entry in enumerate(files, 1):
            parts = Path(entry.filename).parts[strip_parts:]
            if not parts:
                continue
            target = dest.joinpath(*parts)
            if target.exists() and target.is_dir():
                _remove_path(target)
            _ensure_directory(target.parent)
            with archive.open(entry) as src, open(target, "wb") as out:
                shutil.copyfileobj(src, out)
            if index % 50 == 0 or index == len(files):
                print(f"\r  {index}/{len(files)}", end="", flush=True)
        print()

        # Preserve any empty directories that may exist in the archive.
        for entry in entries:
            if not entry.is_dir():
                continue
            parts = Path(entry.filename).parts[strip_parts:]
            if not parts:
                continue
            _ensure_directory(dest.joinpath(*parts))


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="update.py",
        description="TUI auto-updater for Mupen64.",
    )
    parser.add_argument(
        "--channel",
        choices=list(CHANNEL_URLS),
        default=None,
        help="Channel to install. When omitted, the user is prompted to pick one.",
    )
    args = parser.parse_args()

    channel = args.channel if args.channel is not None else prompt_channel()
    url = CHANNEL_URLS[channel]

    mupen_dir = Path(__file__).resolve().parent
    print(f"Channel: {channel}")
    print(f"Target:  {mupen_dir}")
    print()
    print("This will OVERWRITE files in the current directory.")
    if not ask_yes_no("Continue?"):
        print("Update aborted.")
        return 1

    print(f"Downloading '{channel}' artifact...")
    with tempfile.TemporaryDirectory(prefix="mupen64-update-") as tmp:
        archive_path = Path(tmp) / "artifact.zip"
        try:
            download(url, archive_path)
        except (urllib.error.URLError, OSError) as exc:
            print(f"Download failed: {exc}")
            return 1

        if mupen_is_running():
            print()
            print(
                f"{MUPEN_PROCESS_NAME} is running and will be terminated when you press Enter."
            )
            read_line("Press Enter to continue...")
            kill_mupen()
            _wait_for_mupen_exit()

        print()
        try:
            extract_merge(archive_path, mupen_dir)
        except PermissionError as exc:
            print(f"Failed to write files: {exc}")
            print(
                "Make sure mupen and any other applications are closed, then try again."
            )
            return 1

    print("Update complete.")
    return 0


if __name__ == "__main__":
    # Avoid crashing on non-ASCII output when stdout uses a legacy codepage.
    for stream in (sys.stdout, sys.stderr):
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure is not None:
            reconfigure(errors="replace")
    raise SystemExit(main())
