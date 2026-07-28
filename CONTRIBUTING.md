
# Compiling

Only Windows supports compiling the full emulator. However, the core and VCR tests can be (experimentally) compiled on other platforms.

## Windows dependencies

You'll need:
- Visual Studio 2026 (for the MSVC toolchain, Windows SDK, and bundled vcpkg)
  - Ensure a VS developer environment is available (the provided `tools/vsdev-*.cmd` wrappers set this up).
- [Zig 0.15.2+](https://ziglang.org/download/)

## Linux dependencies

You'll need:
- Zig 0.15.2+
- A system C/C++ toolchain for headers (`clang`/`gcc` + libstdc++/libc++)
- `libdeflate`
- `libsafec`
- Catch2 (for tests)

`libsafec` is required outside of Windows as no other C/C++ library implements C11 Annex K, which specifies `strncpy_s` and similar functions.

## Building with Zig

From a VS developer environment on Windows (or via the wrappers):

```sh
# Install third-party deps into build/vcpkg_installed/<triplet>
./tools/vsdev-x64.cmd zig build vcpkg -p build

# Debug build (default). Artifacts land in build/out and build/test/out.
./tools/vsdev-x64.cmd zig build -p build -Dvcpkg_installed=build/vcpkg_installed/x64-windows

# Release build
./tools/vsdev-x64.cmd zig build -p build -Doptimize=ReleaseSafe -Dvcpkg_installed=build/vcpkg_installed/x64-windows

# 32-bit
./tools/vsdev-x86.cmd zig build vcpkg -p build -Dtarget=x86-windows-msvc
./tools/vsdev-x86.cmd zig build -p build -Dtarget=x86-windows-msvc -Dvcpkg_installed=build/vcpkg_installed/x86-windows
```

Useful options:

| Option | Default | Description |
|:--|:--|:--|
| `-Doptimize=` | `Debug` | `Debug`, `ReleaseSafe`, `ReleaseFast`, `ReleaseSmall` |
| `-Dtarget=` | host (MSVC ABI on Windows) | e.g. `x86_64-windows-msvc`, `x86-windows-msvc` |
| `-Denable_dynarec=` | `true` | Dynamic recompiler (x86/x86_64 only) |
| `-Dbuild_win32=` | on when targeting Windows | Win32 frontend + plugins |
| `-Dtests=` | `true` | Build Core.Tests and luatestlib |
| `-Dlink_static=` | `false` | Prefer static vcpkg triplet |
| `-Dvcpkg_installed=` | auto-detect | Path to a vcpkg installed triplet dir |
| `-Dversion_suffix=` | `$VERSION_SUFFIX` | Embedded version suffix |
| `-Dnightly=` | from `$NIGHTLY` | Nightly branding |

### Output layout

```
build/out/mupen64.exe
build/out/plugin/*.dll
build/out/*.dll          # runtime deps from vcpkg (shared builds)
build/test/out/Core.Tests.exe
build/test/out/luatestlib.dll
```

### Visual Studio Code / CLion / Zed

#### Zed
All tasks required for development are available in the task panel.

#### Windows-only
Visual Studio 2026 must be installed on the `C:` drive for the `tools/vsdev-*.cmd` helpers.

## Linux

> **NOTE:** 32-bit builds are not supported on Linux, as this would require far too many extra dependencies to be worth it.

```sh
zig build -Dbuild_win32=false -Doptimize=Debug
zig build test -Dbuild_win32=false
```

# Dependencies
Third-party packages on Windows come from `vcpkg.json` via the `zig build vcpkg` step. Frontend/plugin-only deps should stay gated behind `-Dbuild_win32` in `build.zig`.

# Branching

For contributors: PRs should target `main`. That's all you need to care about.

For maintainers:

Release branches are named `release/<version>` and created from `main`. 

All development happens on `main`. We cherry-pick from `main` into release branches as needed and are free to rewrite history in release branches if necessary.

# Copyright Header

Every non-library file must contain a copyright header with this content:

```
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
```

# Commit Style

Commits to the `main` branch must adhere to [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/#specification).

Only squash merge commits to `main` are allowed.

Example commits:

```
fix(VCR): fix crash when playing a movie while holding B
```

```
feat(ConfigDialog): add plugin config API
```

> [!NOTE]
> For PRs that touch an unreleased feature and thus shouldn't be visible in the changelog, put
> `changelog: skip` in the PR description's footer.

# Code Style

Code formatting must abide by the [.clang-format](https://github.com/mupen64/mupen64-rr-lua/blob/master/.clang-format) file provided in the repository root.

Failure to comply will fail the check-format workflow.

Naming must abide by the [.clang-tidy](https://github.com/mupen64/mupen64-rr-lua/blob/master/.clang-tidy) file provided in the repository root.

# Merge/Release Checklist

Before merging a pull request into main or pushing out a release, verify that:

1. The code is formatted according to the `.clang-format` file
2. The core tests (`src/Core.Tests`) pass
3. The automatic Lua tests (`src/Lua/tests.lua`) pass
4. The manual Lua tests (`src/Lua/manual/*.lua`) pass
5. The docs (`docs/win`) have been kept up-to-date
6. There are no regressions in plugin compatibility (test Jabo's plugins)

# Shipping releases

*Nightly releases*

No work is necessary to ship a nightly release.

It's automated in the [repack](https://github.com/mupen64/repack) repository.

*Stable releases*

To create a stable release:

1. Ensure there's a release branch for the version you're releasing
2. Ensure the version numbers have been bumped in the code
3. On the repo page, navigate to the `Actions` tab and run the pinned `Stable Release` workflow, targeting the release branch
4. Navigate to the release page, find the draft release, double-check that the changelog looks good, and publish it
5. In the [repack](https://github.com/mupen64/repack) repository, run the `Sync` workflow.

# Reading and using Crashlogs

If you have a `mupen.dmp`, open it in WinDbg and run `!analyze-v`.

If you only have the stacktrace from `mupen.log`:

1. Identify the faulting address
2. Open x32dbg
3. Open the "Go to" dialog by pressing Ctrl + G
4. Navigate to `0x00400000` + `[Your Address]`

# TAS Plugins and Plugin Compatibility

The "TAS" plugins are our first-party plugins that aim to be lightweight and fast.

They're tied to their contemporary version of Mupen and are not guaranteed to be compatible with older or newer versions.

While Mupen is compatible with any Zilmar spec plugin (e.g. Jabo's plugins, GLideN64), we plan to prioritize our first-party plugins by moving to a private plugin API in the future.
Support for Zilmar spec support will eventually be provided only via a shim layer (cf. [#670](https://github.com/mupen64/mupen64-rr-lua/issues/670))

## Developer Guidelines

### Naming

The plugin's friendly name should follow the schema:

`[Plugin Name] [Version] [x64] [Debug]` (e.g.: `TAS Input 2.0.0`, `TAS Input 2.0.0 x64 Debug`)

### Initialization

Keep `DllMain` as simple as possible; do not initialize SDL, DirectInput, or any other external libraries.

Initialize libraries in `RomOpen` and - if possible - do it only once.

### Configuration

Write persistent config to the filesystem as JSON, ideally next to the mupen executable.
