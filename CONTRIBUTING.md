
# Compiling

Only Windows is supported for now, though the CMake infrastructure is intended to ease the development of cross-platform code (for whoever decides to work on that).

## CMake Options
| OPTION                    | DESCRIPTION                                                           |
|:-------------------------:|-----------------------------------------------------------------------|
| `MUPEN64RR_USE_SANITIZER` | Specifies a sanitizer to compile with. [`{OFF, ASAN}`, default `OFF`] |

## Windows/CMake
You'll need:
- Visual Studio 2026 (for the compiler, CMake, Ninja and vcpkg)

In order for the compiler to work, you'll need to be in a VS developer environment. Then, simply use the provided `vcpkg-win64-x86` preset to compile and build. If you want to change any settings, do so on the command line or via `CMakeUserPresets.json`.
```sh
cmake --preset vcpkg-win64-x86
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

Make sure to set the CMake profile to use the `vcpkg-win64-x86` preset, enabling it if needed.

If you aren't presented with a CMake profile selection dialog on startup, you can change the active profile by going to `File -> Settings -> Build, Execution, Deployment -> CMake`.

### Zed

All tasks required for development are available in the task panel.

Visual Studio 2026 must be installed on the `C:` drive.

# Dependencies
When adding CMake dependencies, ensure that dependencies specific to the frontend and/or plugins are wrapped inside an `if()` block. this will ensure cross-platform compatibility when the time comes for that.

> **Note:** the Windows GUI components are gated behind `MUPEN64RR_BUILD_WIN32`. 

```cmake
# example: GLEW and SDL3 are specifically for the windows
if (MUPEN64RR_BUILD_WIN32)
  find_package(glew CONFIG REQUIRED) 
  find_package(SDL3 CONFIG REQUIRED) 
endif()
```

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

# Plugin Guidelines

- Give your plugin a descriptive name.
    1. The friendly name should be formatted as follows: `[Plugin Name] [Version] [x64] [Debug]`
        a. e.g.: `TASInput 2.0.0`, `TASInput 2.0.0 x64 Debug`
- Do as little initialization work in `DllMain` as possible. Do it all in `RomOpen` and cache the results.
    1. Watch out for implicit COM initialization through DirectInput!
- Write persistent config to the registry, not the filesystem.
    1. Play fair, don't pollute the user's Mupen directory if possible.

# Merge/Release Checklist

Before merging a pull request into main or pushing out a release, verify that:

1. The code is formatted according to the `.clang-format` file
2. The core tests (`test/Core.Tests`) pass
3. The automatic Lua tests (`test/lua/tests.lua`) pass
4. The manual Lua tests (`test/lua/manual/*.lua`) pass
5. The general docs have been kept up-to-date
6. The Lua docs have been rebuilt (`.\docs\lua\build_documentation.bat`)
7. There are no regressions in plugin compatibility (test Jabo's plugins)

### Release

1. Generate a changelog using [git-cliff](https://git-cliff.org/)

    ```
    git-cliff -o CHANGELOG.md --unreleased
    ```

2. Write a summary of the release in the `# Summary` section of the changelog.
3. Create a GitHub release with the following parameters:

    Title: [version number]

    Description: [CHANGELOG.md]

    Files: [latest `mupen64.exe` build artifact]


# Reading and using Crashlogs

If you have a `mupen.dmp`, open it in WinDbg and run `!analyze-v`.

If you only have the stacktrace from `mupen.log`:

1. Identify the faulting address
2. Open x32dbg
3. Open the "Go to" dialog by pressing Ctrl + G
4. Navigate to `0x00400000` + `[Your Address]`
