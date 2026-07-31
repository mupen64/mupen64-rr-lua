#
#  Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#

import argparse
import fnmatch
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

EXTENSIONS = {".cpp", ".hpp"}


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


def git_available() -> bool:
    try:
        subprocess.run(["git", "--version"], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def load_clang_format_ignore(root: Path) -> set[str]:
    patterns: set[str] = set()
    path = root / ".clang-format-ignore"
    if path.is_file():
        for raw in path.read_text(encoding="utf-8").splitlines():
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            patterns.add(line)
    return patterns


def _is_git_ignored(root: Path, rel: Path) -> bool:
    proc = subprocess.run(
        ["git", "check-ignore", "-q", str(rel)],
        cwd=root,
        capture_output=True,
    )
    return proc.returncode == 0


def _walk(root: Path, use_git: bool) -> list[Path]:
    paths: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        if ".git" in dirnames:
            dirnames.remove(".git")
        for fn in filenames:
            rel = (Path(dirpath) / fn).relative_to(root)
            if use_git and _is_git_ignored(root, rel):
                continue
            paths.append(rel)
    return paths


def collect_sources(root: Path, ignore_patterns: set[str]) -> list[Path]:
    use_git = git_available()
    if use_git:
        try:
            result = subprocess.run(
                ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
                cwd=root,
                capture_output=True,
                text=True,
                check=True,
            )
            rel_paths = [
                Path(line.strip())
                for line in result.stdout.splitlines()
                if line.strip()
            ]
        except subprocess.CalledProcessError:
            rel_paths = _walk(root, use_git)
    else:
        rel_paths = _walk(root, use_git)

    sources: list[Path] = []
    for rel in rel_paths:
        if rel.suffix.lower() not in EXTENSIONS:
            continue
        if any(fnmatch.fnmatch(rel.as_posix(), pat) for pat in ignore_patterns):
            continue
        sources.append(root / rel)
    return sources


def format_file(path: Path, clang_format: str) -> tuple[Path, int, str]:
    cmd = [clang_format, "-i", str(path)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    return path, proc.returncode, proc.stderr


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--clang-format",
        default="clang-format",
        help="Path to the clang-format executable (default: clang-format on PATH).",
    )
    parser.add_argument(
        "--jobs",
        "-j",
        type=int,
        default=os.cpu_count() or 1,
        help="Number of parallel workers (default: number of CPUs).",
    )
    args = parser.parse_args()

    root = project_root()
    ignore_patterns = load_clang_format_ignore(root)
    sources = collect_sources(root, ignore_patterns)

    if not sources:
        print("No .cpp/.hpp files to format.")
        return 0

    verb = "Formatting"
    print(f"{verb} {len(sources)} file(s) with {args.jobs} worker(s)...")

    failed: list[Path] = []
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(format_file, p, args.clang_format): p
            for p in sources
        }
        for future in as_completed(futures):
            path, returncode, stderr = future.result()
            if returncode != 0:
                failed.append(path)
                sys.stderr.write(f"{path}: {stderr}\n")

    if failed:
        print(f"Failed to format {len(failed)} file(s).")
        return 1

    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
