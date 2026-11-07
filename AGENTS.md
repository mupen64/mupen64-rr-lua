# AGENTS.md

mupen64-rr-lua: an N64 emulator (Nintendo 64) focused on TAS tooling, in C++ with an embedded Lua engine.
Full contribution rules live in `CONTRIBUTING.md`; this file is a quick brief for coding agents.

## Build

- CMake + Ninja + clang. Use a configure preset from `CMakePresets.json`, then build:
  - Linux (core + Qt frontend): `cmake --preset sys-linux && cmake --build build`
  - Windows host: `vcpkg-win64-x64` (run from a Visual Studio developer console)
  - Cross-compile Linux to Windows: `mingw-linux-x64` / `mingw-linux-x86`
  - Release variants: append `-release` to the preset name.
- Only Windows builds the full emulator; on other platforms the core and VCR tests compile.
- GUI frontends are gated by options: `MUPEN64RR_BUILD_WIN32` (default on Windows) and `MUPEN64RR_BUILD_QT6` (default on Linux). Other options are documented at the top of the root `CMakeLists.txt`.

## Test

- `ctest --test-dir build` runs the core tests (`src/Core.Tests`) and VCR tests.
- Automatic Lua tests: `src/Lua/tests.lua`. Manual Lua tests: `src/Lua/manual/*.lua`.
- Before merging, see the merge/release checklist in `CONTRIBUTING.md` (format, core tests, Lua tests, docs).

## Layout

- `src/Core`, `src/Common`: emulator core and shared code.
- `src/Views.*`, `src/Common.Views`: GUI frontends (Win32, Qt6).
- `src/Lua`: embedded Lua engine and its tests. `src/Plugins.*`: plugins (audio/video/RSP/input).
- `vendor/`: small vendored libraries, each exposed as a `vendor::<name>` CMake alias target.
- Larger dependencies go through `find_package`: `vcpkg.json` on Windows, system packages on Linux. Frontend-specific deps must stay inside `if()` blocks.

## Conventions

- Formatting is enforced by CI (`.github/workflows/check-format.yml`) against the root `.clang-format`; naming follows `.clang-tidy`, except `Views.Qt6` which follows Qt conventions. Run clang-format before committing.
- Copyright header on every non-library file (`SPDX-License-Identifier: GPL-2.0-or-later`); see `CONTRIBUTING.md` for the exact text. Credits belong in `NOTICE`, not in file headers.
- Commits to `main` follow Conventional Commits (`feat(ConfigDialog): ...`); only squash merges land on `main`.
- PRs target `main`.
