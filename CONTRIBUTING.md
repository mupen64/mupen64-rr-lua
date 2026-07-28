
# Compiling

The emulator can be compiled in full both on Windows and Linux. Compiling on Linux requires MinGW.

Make sure you're using Visual Studio Code or Zed (or any IDE compatible with their task files), as we'll be using the pre-configured build tasks.

## Windows Setup

You'll need:

- Visual Studio 2026 (for the MSVC toolchain, Windows SDK, and bundled vcpkg)
- Zig

## Linux Setup

This guide is written for Arch Linux, but should be equivalent for other distributions.

1. Install the dependencies
```sh
sudo pacman -S --needed base-devel zig mingw-w64-gcc nasm vcpkg
yay -S libsafec

git clone https://github.com/microsoft/vcpkg ~/.local/share/vcpkg
~/.local/share/vcpkg/bootstrap-vcpkg.sh -disableMetrics
```

## Building

1. Run the `vcpkg install (Win64)` task
2. Run the `generate compile_commands.json (Win64)` task
3. Run the `build (Win64)` task

Useful flags:

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
