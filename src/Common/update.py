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
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

CHANNEL_URLS = {
    "stable-w64": "https://github.com/mupen64/repack-stable-w64/archive/refs/heads/main.zip",
    "stable-w32": "https://github.com/mupen64/repack-stable-w32/archive/refs/heads/main.zip",
    "nightly-w64": "https://github.com/mupen64/repack-nightly-w64/archive/refs/heads/main.zip",
    "nightly-w32": "https://github.com/mupen64/repack-nightly-w32/archive/refs/heads/main.zip",
}

# Very short descriptions shown next to each channel in the selection prompt.
CHANNEL_DESCRIPTIONS = {
    "stable-w64": "Stable 64-bit. Recommended for most users.",
    "stable-w32": "Stable 32-bit. Only for compatibility with legacy plugins (i.e. Jabo's).",
    "nightly-w64": "Nightly 64-bit.",
    "nightly-w32": "Nightly 32-bit.",
}

# The mupen executable that gets terminated before an update is applied.
MUPEN_PROCESS_NAME = "mupen64.exe"

DOWNLOAD_CHUNK_SIZE = 256 * 1024

PROGRESS_BAR_WIDTH = 40


# ---------------------------------------------------------------------------
# TUI colors
# ---------------------------------------------------------------------------


class _Palette:
    """ANSI escape codes used for the splash screen and printouts."""

    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    CYAN = "\033[36m"
    GRAY = "\033[90m"
    BOLD = "\033[1m"
    RESET = "\033[0m"


def _enable_windows_vt() -> None:
    """Enables ANSI escape processing in the Windows console, if possible."""
    if os.name != "nt":
        return
    # Python 3.13+
    enable_vt = getattr(os, "enable_virtual_terminal_processing", None)
    if enable_vt is not None:
        try:
            enable_vt()
            return
        except OSError:
            pass
    # Fallback: set ENABLE_VIRTUAL_TERMINAL_PROCESSING on the console mode.
    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        handle = kernel32.GetStdHandle(-11)  # STD_OUTPUT_HANDLE
        mode = ctypes.c_uint32()
        if kernel32.GetConsoleMode(handle, ctypes.byref(mode)) and mode.value:
            kernel32.SetConsoleMode(handle, mode.value | 0x0004)
    except (AttributeError, OSError):
        pass


def _color_enabled() -> bool:
    return sys.stdout.isatty() and not os.environ.get("NO_COLOR")


_COLOR = _color_enabled()


def paint(text: str, *codes: str) -> str:
    """Wraps `text` in ANSI escape codes, unless color output is disabled."""
    if not _COLOR or not codes:
        return text
    return "".join(codes) + text + _Palette.RESET


def channel_color(channel: str) -> str:
    """Returns the color used to display a channel name."""
    return _Palette.GREEN if channel.startswith("stable") else _Palette.YELLOW


def read_line(prompt: str = "") -> str:
    """Reads a line from stdin, tolerating a closed/non-interactive stdin."""
    try:
        return input(prompt)
    except EOFError:
        print()
        return ""


def ask_yes_no(prompt: str) -> bool:
    """Asks the user a yes/no question, retrying until a valid answer is given."""
    while True:
        answer = read_line(f"{prompt} [y/N] ").strip().lower()
        if answer in ("y", "yes"):
            return True
        if answer in ("n", "no", ""):
            return False
        print(paint("Please answer 'y' or 'n'.", _Palette.YELLOW))


def prompt_channel() -> str:
    """Prompts the user to pick an update channel from the list."""
    names = list(CHANNEL_URLS)
    width = max(len(name) for name in names)
    print(paint("Available channels:", _Palette.BOLD))
    for index, name in enumerate(names, 1):
        print(
            f"  {paint(f'{index})', _Palette.CYAN, _Palette.BOLD)} "
            f"{paint(name.ljust(width), channel_color(name))} "
            f"{paint(f'- {CHANNEL_DESCRIPTIONS[name]}', _Palette.GRAY)}"
        )
    while True:
        raw = read_line(
            paint(f"Select a channel [1-{len(names)}]: ", _Palette.CYAN)
        ).strip()
        if not raw:
            print(paint("Update aborted.", _Palette.YELLOW))
            raise SystemExit(1)
        if not raw.isdigit():
            print(paint("Please enter a number.", _Palette.YELLOW))
            continue
        index = int(raw)
        if 1 <= index <= len(names):
            return names[index - 1]
        print(
            paint(f"Please enter a number between 1 and {len(names)}.", _Palette.YELLOW)
        )


def format_size(num_bytes: int) -> str:
    return f"{num_bytes / (1024 * 1024):.1f} MiB"


def _progress_bar(downloaded: int, total: int) -> str:
    """Renders a progress bar line, e.g. `[####------] 40%  10.0 MiB / 25.0 MiB`."""
    filled = min(PROGRESS_BAR_WIDTH, PROGRESS_BAR_WIDTH * downloaded // total)
    bar = "[" + "#" * filled + " " * (PROGRESS_BAR_WIDTH - filled) + "]"
    percent = min(100, downloaded * 100 // total)
    return f"{bar} {percent}%  {format_size(downloaded)} / {format_size(total)}"


def download(url: str, dest: Path) -> None:
    """Downloads `url` to `dest`, printing a progress bar readout."""
    print(paint(f"Downloading {url}", _Palette.CYAN))
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
                        paint(
                            f"\r  {_progress_bar(downloaded, total)}",
                            _Palette.CYAN,
                        ),
                        end="",
                        flush=True,
                    )
                    last_percent = percent
        if total > 0:
            print(paint(f"\r  {_progress_bar(total, total)}", _Palette.CYAN))
        else:
            print(paint(f"  Downloaded {format_size(downloaded)}.", _Palette.CYAN))


def _linux_mupen_pids() -> list[str]:
    """Returns the PIDs of running mupen64.exe processes (Linux/Wine).

    Wine names the Linux process after the PE image, so the process shows up
    with the exact name mupen64.exe. Match on the process name (comm) rather
    than the command line to avoid false positives from unrelated processes.
    """
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
    """Returns True if a mupen64.exe process is currently running.

    On Windows this is detected with tasklist. On Linux, mupen runs under Wine
    and shows up as a native mupen64.exe process, matched with pgrep.
    """
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
    """Forcefully terminates all running mupen64.exe processes."""
    print(paint(f"Terminating {MUPEN_PROCESS_NAME}...", _Palette.YELLOW))
    if sys.platform == "win32":
        subprocess.run(["taskkill", "/IM", MUPEN_PROCESS_NAME, "/F"], check=False)
        return
    # taskkill is a Windows tool; on Linux signal the Wine-hosted mupen64.exe
    # processes directly.
    pids = _linux_mupen_pids()
    if pids:
        subprocess.run(["kill", "-9", *pids], check=False)


def _wait_for_mupen_exit(timeout: float = 10.0) -> None:
    """Blocks until no mupen64.exe process remains.

    On Windows this is a no-op because taskkill /F already waits for the
    process to terminate. On Linux, kill only sends the signal, so poll until
    the process is actually gone before touching its files.
    """
    if sys.platform == "win32":
        return
    import time

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if not mupen_is_running():
            return
        time.sleep(0.2)
    print(
        paint(
            "Warning: mupen64.exe did not exit in time; continuing anyway.",
            _Palette.YELLOW,
        )
    )


def _remove_path(path: Path) -> None:
    """Removes a file or directory tree at `path`."""
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path)
    else:
        path.unlink()


def _ensure_directory(path: Path) -> None:
    """Creates `path` as a directory, replacing any file that blocks the way."""
    if path.exists():
        if path.is_dir():
            return
        _remove_path(path)
    parent = path.parent
    if parent != path:
        _ensure_directory(parent)
    path.mkdir()


def extract_merge(zip_path: Path, dest: Path) -> None:
    """Extracts a channel archive into `dest`, overwriting existing files.

    GitHub wraps archives in a single top-level folder; that folder is stripped
    so the repack contents land directly in the mupen directory. Existing files
    are overwritten, directories are created as needed, and conflicts between
    files and directories are resolved in favor of the archive.
    """
    with zipfile.ZipFile(zip_path) as archive:
        entries = [entry for entry in archive.infolist() if entry.filename]
        if not entries:
            return

        strip_parts = 0
        first_part = Path(entries[0].filename).parts[0]
        if all(Path(entry.filename).parts[0] == first_part for entry in entries):
            strip_parts = 1

        files = [entry for entry in entries if not entry.is_dir()]
        print(paint(f"Extracting {len(files)} file(s) into {dest} ...", _Palette.CYAN))

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
                print(
                    paint(f"\r  {index}/{len(files)}", _Palette.CYAN),
                    end="",
                    flush=True,
                )
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

    mupen_dir = Path(__file__).resolve().parent
    print(f"{paint('Target:', _Palette.BOLD)} {mupen_dir}")

    if args.channel is None:
        print()
    channel = args.channel if args.channel is not None else prompt_channel()
    url = CHANNEL_URLS[channel]

    print()
    print(
        paint(
            "This will OVERWRITE files in the current directory.",
            _Palette.YELLOW,
            _Palette.BOLD,
        )
    )
    if not ask_yes_no("Continue?"):
        print(paint("Update aborted.", _Palette.YELLOW))
        return 1

    print(paint(f"Downloading '{channel}' artifact...", _Palette.CYAN))
    with tempfile.TemporaryDirectory(prefix="mupen64-update-") as tmp:
        archive_path = Path(tmp) / "artifact.zip"
        try:
            download(url, archive_path)
        except (urllib.error.URLError, OSError) as exc:
            print(paint(f"Download failed: {exc}", _Palette.RED))
            return 1

        if mupen_is_running():
            print()
            print(
                paint(
                    f"{MUPEN_PROCESS_NAME} is running and will be terminated when you press Enter.",
                    _Palette.YELLOW,
                    _Palette.BOLD,
                )
            )
            read_line(paint("Press Enter to continue...", _Palette.YELLOW))
            kill_mupen()
            _wait_for_mupen_exit()

        print()
        try:
            extract_merge(archive_path, mupen_dir)
        except PermissionError as exc:
            print(paint(f"Failed to write files: {exc}", _Palette.RED))
            print(
                paint(
                    "Make sure mupen and any other applications are closed, then try again.",
                    _Palette.YELLOW,
                )
            )
            return 1

    print(paint("Update complete.", _Palette.GREEN, _Palette.BOLD))
    return 0


if __name__ == "__main__":
    _enable_windows_vt()
    _COLOR = _color_enabled()
    # Avoid crashing on non-ASCII output when stdout uses a legacy codepage.
    for stream in (sys.stdout, sys.stderr):
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure is not None:
            reconfigure(errors="replace")
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print()
        print(paint("Update aborted.", _Palette.YELLOW))
        raise SystemExit(130)
