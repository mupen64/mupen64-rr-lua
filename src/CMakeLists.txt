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
    add_subdirectory(Plugins.Audio.Dummy)
    add_subdirectory(Plugins.Input.Dummy)
    add_subdirectory(Plugins.RSP.Dummy)
    add_subdirectory(Plugins.Video.Dummy)

    # TAS RSP
    add_subdirectory(Plugins.RSP.TAS)
endif()
