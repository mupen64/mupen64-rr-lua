# Copyright Header

Every non-library file must contain the following header:

```cpp
/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
```

# Compiling

Open the solution with your IDE of choice and build the solution.

Recommended IDEs are:

- [Visual Studio 2022](https://visualstudio.microsoft.com/vs/)
- [Rider](https://www.jetbrains.com/rider/)

For Rider users:

It's recommended to enable "Use external console" (Run > Modify Run Configuration...).

# Commit Style

Commits to the `main` branch must adhere to [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/#specification).

Commits made on other branches don't have to adhere to any specific commit style - the merge commits have to, though.

Example commits:

```
fix(VCR): fix crash when playing a movie while holding B
```

```
feat(ConfigDialog): add plugin config API
```

# Changelogs

Generate changelogs using [git-cliff](https://git-cliff.org/).

# Program Structure

Mupen64 is currently divided into two parts: its Core, and its Windows GUI.

## <img src="https://github.com/user-attachments/assets/fa3b86d8-6bcf-4b65-a575-4ea2930a516c" width="64" align="center"/> Core 

The core implements emulation functionality and defines an API which allows it to be driven externally.

## <img src="https://github.com/user-attachments/assets/28439517-0e7a-41d6-829d-c2bd2f065d14" width="64" align="center"/> Views.Win32

The Win32 implementation of a Mupen64 GUI.

# Reading and using Crashlogs

## 1.2.0 and newer

1. Open the `mupen.dmp` file in WinDbg
2. Run `!analyze -v`

## Pre-1.2.0

1. Find the stacktrace

```
[2025-02-06 18:58:07.911] [View] [critical] Stacktrace:
[2025-02-06 18:58:07.946] [View] [critical] mupen64_119_9_avx2+0xA2AB6
[2025-02-06 18:58:07.946] [View] [critical] mupen64_119_9_avx2+0xD304A
[2025-02-06 18:58:07.950] [View] [critical] ntdll!RtlGetFullPathName_UEx+0x13F
[2025-02-06 18:58:07.950] [View] [critical] ntdll!RtlGetFullPathName_UEx+0x7B
```

2. Identify the frame you want to look at

```
0xA2AB6
```

3. Open x32dbg

4. Open the "Go to" dialog by pressing Ctrl + G

5. Navigate to `0x00400000` + `[Your Address]`

# Code Style

Code formatting must, to the furthest possible extent, abide by the [.clang-format](https://github.com/mkdasher/mupen64-rr-lua-/blob/master/.clang-format) file provided in the repository root.


