#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]


# USEFUL SETTINGS
# ============================
set(CMAKE_SHARED_MODULE_PREFIX "")

# COMMON LIBRARIES
# ============================

# common headers and PCH
add_subdirectory(Common)
# core API and headers
add_subdirectory(Core)


# CLI VIEW
# ============================

add_subdirectory(Views.CLI)

# BUNDLED PLUGINS
# ============================

# common settings
include("${CMAKE_CURRENT_LIST_DIR}/Plugins.Common.cmake")

# dummy plugins
add_subdirectory(Plugins.DummyAudio)
add_subdirectory(Plugins.DummyInput)
add_subdirectory(Plugins.DummyVideo)


if (MUPEN64RR_BUILD_WIN32)
    # VIEW
    # ============================

    add_subdirectory(Common.Win32)
    add_subdirectory(Views.Win32)

    # BUNDLED PLUGINS
    # ============================

    # common settings
    include("${CMAKE_CURRENT_LIST_DIR}/Plugins.Win32.Common.cmake")

    # TAS plugins
    add_subdirectory(Plugins.Win32.TASVideo)
    add_subdirectory(Plugins.Win32.TASInput)
    add_subdirectory(Plugins.Win32.TASAudio)
    add_subdirectory(Plugins.Win32.TASRSP)
endif()

# TESTS
# ============================

add_subdirectory(Core.Tests)
add_subdirectory(Lua.TestLib)
