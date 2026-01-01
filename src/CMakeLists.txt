#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

# COMMON LIBRARIES
# ============================

# common headers and PCH
add_subdirectory(Common)
# core API and headers
add_subdirectory(Core)


if (MUPEN64RR_BUILD_WIN32)
    # VIEW
    # ============================

    add_subdirectory(Views.Win32)

    # BUNDLED PLUGINS
    # ============================
    
    # common settings
    include("${CMAKE_CURRENT_LIST_DIR}/Plugins.Common.cmake")

    # dummy plugins
    add_subdirectory(Plugins.Win32.DummyAudio)
    add_subdirectory(Plugins.Win32.DummyInput)
    add_subdirectory(Plugins.Win32.DummyRSP)
    add_subdirectory(Plugins.Win32.DummyVideo)

    # TAS plugins
    add_subdirectory(Plugins.Win32.TASVideo)
    add_subdirectory(Plugins.Win32.TASInput)
    add_subdirectory(Plugins.Win32.TASAudio)
    add_subdirectory(Plugins.Win32.TASRSP)
endif()
