#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]


# USEFUL SETTINGS
# ============================

# we don't want our plugins to have the "lib" prefix
set(CMAKE_SHARED_MODULE_PREFIX "")

# COMMON LIBRARIES
# ============================

# common headers and PCH
add_subdirectory(Common)
# core API and headers
add_subdirectory(Core)




# BUNDLED PLUGINS
# ============================

# common settings
include("${CMAKE_CURRENT_LIST_DIR}/Plugins.Common.cmake")

# dummy plugins
add_subdirectory(Plugins.DummyAudio)
add_subdirectory(Plugins.DummyInput)
add_subdirectory(Plugins.DummyVideo)

# SDL audio plugin
add_subdirectory(Plugins.TASAudio)
# HLE RSP plugin (core cannot live without a functioning RSP)
add_subdirectory(Plugins.TASRSP)

# WIN32
# ============================

if (MUPEN64RR_BUILD_WIN32)
    # VIEW
    # ============================

    add_subdirectory(Common.Win32)
    add_subdirectory(Common.Views)
    add_subdirectory(Views.Win32)

    # BUNDLED PLUGINS
    # ============================

    # common settings
    include("${CMAKE_CURRENT_LIST_DIR}/Plugins.Win32.Common.cmake")

    # Windows-based TAS plugins.
    # TASVideo and TASInput rely heavily on Windows APIs to work.
    # Note that we can't port config dialogs until we settle on a GUI frontend framework.
    add_subdirectory(Plugins.Win32.TASVideo)
    add_subdirectory(Plugins.Win32.TASInput)
endif()

# CLI VIEW
# ============================

if (MUPEN64RR_BUILD_CLI)
    add_subdirectory(Views.CLI)
endif()

# TESTS
# ============================

add_subdirectory(Core.Tests)
add_subdirectory(Lua.TestLib)
