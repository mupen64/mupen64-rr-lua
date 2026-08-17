# Compiling

Only Windows supports compiling the full emulator. However, the core and VCR tests can be (experimentally) compiled on other platforms.

## Windows dependencies

Install Visual Studio 2026 with:
- MSVC
- Windows SDK
- LLVM tools
- CMake
- Ninja
- vcpkg

Most of these are included by default on the **Desktop Development with C++** workload.

### Qt frontend 
You'll also need a Qt installation to build the Qt frontend; install with:
- MSVC *(note: these are the Qt libraries as built for MSVC)*

Set the environment variable `CMAKE_PREFIX_PATH` to `C:\Qt\6.11.1\msvc2022_64`. If you installed Qt to a directory other than `C:\Qt`, change that path accordingly.

If you want to save space, you can skip installing Qt Creator, CMake, Ninja; the only thing needed from the Qt install is the prebuilt libraries.

## Linux dependencies

Arch (and relatives):
```sh
# core dependencies
sudo pacman -S --needed base-devel cmake ninja clang pkgconf catch2 libdeflate lz4 lua
yay -S --needed libsafec

# Qt frontend
sudo pacman -S --needed qt6-base qt6-declarative

# MinGW cross-compilation
sudo pacman -S mingw-w64-gcc vcpkg
source /etc/profile.d/vcpkg.sh
```

Find equivalent packages for your distro if it isn't listed here.

## CMake Presets and Options
Compiling is as easy as using one of the provided configure presets. All platforms generally use `clang` as the compiler and `Ninja` as the generator. All presets have release-build counterparts bearing a `-release` suffix (`vcpkg-win64-x64-release`, `sys-linux-release`, etc.).

|Preset|Platform|
|:--|:--|
|`vcpkg-win64-x64`|**64-bit target** on **64-bit Windows** host, dependencies via `vcpkg`|
|`vcpkg-win64-x86`|**32-bit** target on **64-bit Windows** host, dependencies via `vcpkg`|
|`sys-linux`|compile for **Linux** host, dependencies via system|
|`mingw-linux-x64`|**64-bit** MinGW cross-compile from **Linux** to **Windows**, dependencies via system or optional `vcpkg`|
|`mingw-linux-x86`|**32-bit** MinGW cross-compile from **Linux** to **Windows**, dependencies via system or optional `vcpkg`|

The two available GUI frontends are:
- Win32 (option `MUPEN64RR_BUILD_WIN32`): enabled by default on Windows, disabled elsewhere
- Qt (***experimental***, option `MUPEN64RR_BUILD_QT6`): enabled by default on Linux, disabled elsewhere

Additional options are described at the top of the root `CMakeLists.txt` file; see there or use `ccmake` for more information.

### Command line
This should be familiar to CMake users. *(Windows)* Ensure this is run in a Visual Studio developer console.
```sh
# building
cmake --preset "<preset-name>"
cmake --build build
# testing
ctest --test-dir build
```

### Visual Studio Code + CMake Tools
- Configure presets should be made available via CMake Tools, see above.
- *(Windows)* Add `"cmake.useVsDeveloperEnvironment": "always"` in your workspace settings to ensure CMake Tools sets up a Visual Studio environment.

### CLion
- Make sure to set the CMake profile to use the desired preset (`vcpkg-win64-x86` or `vcpkg-win64-x64`), enabling it if needed.
- If you aren't presented with a CMake profile selection dialog on startup, you can change the active profile by going to `File -> Settings -> Build, Execution, Deployment -> CMake`.

### Zed
- All tasks required for development are available in the task panel.
- Visual Studio 2026 must be installed on the `C:` drive.

## MinGW cross-compilation
Compile and build using the provided `mingw-linux-*` presets. MinGW runtime DLLs (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`) will be automatically copied to the output directory at build time.

# Dependencies

## Major dependencies

If you need to include a large cross-platform library, it should be found via `find_package`. By platform:
  - **Windows**: add the dependency to `vcpkg.json`
  - **Linux**: rely on the system package.

> **Note:** Qt is an exception on Windows, as it should be provided by a Qt installation.

Dependencies should be provided in this fashion if a package is available for it on both **the latest Ubuntu LTS** (currently 26.04 "resolute") and **the latest Fedora release**.

Ensure that dependencies specific to a frontend and/or plugins are wrapped inside an `if()` block.

> **Note:** the Windows GUI components are gated behind `MUPEN64RR_BUILD_WIN32`. 

```cmake
# example: GLEW and SDL3 are specifically for the windows
if (MUPEN64RR_BUILD_WIN32)
  find_package(glew CONFIG REQUIRED) 
  find_package(SDL3 CONFIG REQUIRED) 
endif()
```

## Vendored dependencies

Smaller libraries may be directly vendored in the `vendor/` subdirectory. Make sure to setup an alias target for each library.

```cmake
add_subdirectory(argh)
add_library(vendor::argh ALIAS argh)
```


# Branching

For contributors: PRs should target `main`. That's all you need to care about.

For maintainers:

Release branches are named `release/<version>` and created from `main`. 

All development happens on `main`. We cherry-pick from `main` into release branches as needed and are free to rewrite history in release branches if necessary.

# Copyright Header

Every non-library file must contain a copyright header with this content:

```
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
```

The header is the same everywhere; only the comment syntax changes per file type. Project-specific credits to
original authors belong in the `NOTICE` file, not in per-file headers.

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

The "TAS" plugins are our first-party plugins that aim to be lightweight and fast. They're tied to their contemporary version of Mupen and are not guaranteed to be compatible with older or newer versions.

Mupen64 remains compatible with Zilmar-spec plugin (e.g. Jabo's plugins, GLideN64), however, these are only supported via a shim on the Win32 frontend. The upcoming Qt frontend will likely not support Zilmar-spec plugins, as the additional development effort to support it seems too great to be helpful.