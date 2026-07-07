
# Compiling

Only Windows is supported for now, though the CMake infrastructure is intended to ease the development of cross-platform code (for whoever decides to work on that).

## CMake Options
| OPTION                    | DESCRIPTION                                                           |
|:-------------------------:|-----------------------------------------------------------------------|
| `MUPEN64RR_USE_SANITIZER` | Specifies a sanitizer to compile with. [`{OFF, ASAN}`, default `OFF`] |

## Windows/CMake
You'll need:
- Visual Studio 2026 (for the compiler, CMake, Ninja and vcpkg)

In order for the compiler to work, you'll need to be in a VS developer environment. Then, simply use one of the provided presets to compile and build. If you want to change any settings, do so on the command line or via `CMakeUserPresets.json`.

For a 32-bit build:
```sh
cmake --preset vcpkg-win64-x86
cmake --build build
```

For a 64-bit build:
```sh
cmake --preset vcpkg-win64-x64
cmake --build build
```

The core VCR tests are integrated with CMake, so running the tests is easy:
```sh
ctest --test-dir build
```

Presets have been provided for building and testing. These are intended for IDEs, so that they can properly autodetect things. Feel free to contribute IDE launch settings as appropriate.


### Visual Studio Code + CMake Tools
You'll need to enable `"cmake.useVsDeveloperEnvironment": "always"` in your workspace settings to convince CMake Tools to set up a VS developer environment.

### CLion

Make sure to set the CMake profile to use the desired preset (`vcpkg-win64-x86` or `vcpkg-win64-x64`), enabling it if needed.

If you aren't presented with a CMake profile selection dialog on startup, you can change the active profile by going to `File -> Settings -> Build, Execution, Deployment -> CMake`.

### Zed

All tasks required for development are available in the task panel.

Visual Studio 2026 must be installed on the `C:` drive.

# Dependencies
When adding CMake dependencies, ensure that dependencies specific to the frontend and/or plugins are wrapped inside an `if()` block. this will ensure cross-platform compatibility when the time comes for that.

> **Note:** the Windows GUI components are gated behind `MUPEN64RR_BUILD_WIN32`. 

```cmake
# example: GLEW, SDL3, and Blend2d are specifically for the windows
if (MUPEN64RR_BUILD_WIN32)
  find_package(glew CONFIG REQUIRED) 
  find_package(SDL3 CONFIG REQUIRED) 
  find_package(blend2d CONFIG REQUIRED) 
endif()
```

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
5. The docs (`src/Website/static/docs/win`) have been kept up-to-date
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
