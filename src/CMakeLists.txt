#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]


# USEFUL SETTINGS
# ============================

# COMMON LIBRARIES
# ============================

# common headers and PCH
add_subdirectory(Common)

function(mupen64rr_reuse_common_pch target)
    target_precompile_headers("${target}" REUSE_FROM Mupen64RR_Common)
endfunction()
# core API and headers
add_subdirectory(Core)
# shared view functions
add_subdirectory(Common.Views)

# BUNDLED PLUGINS
# ============================

# common settings
include("${CMAKE_CURRENT_LIST_DIR}/Plugins.Common.cmake")

# dummy plugins
add_subdirectory(Plugins.DummyAudio)
add_subdirectory(Plugins.DummyInput)
add_subdirectory(Plugins.DummyVideo)

# SDL video plugin
add_subdirectory(Plugins.TASVideo)
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

    add_library(Mupen64RR_Views_Win32_Headers INTERFACE
        "Views.Win32/include/Views.Win32/ZESpec.h"
    )
    target_compile_features(Mupen64RR_Views_Win32_Headers INTERFACE cxx_std_26)
    target_include_directories(Mupen64RR_Views_Win32_Headers INTERFACE "Views.Win32/include")

    # BUNDLED PLUGINS
    # ============================

    # common settings
    include("${CMAKE_CURRENT_LIST_DIR}/Plugins.Win32.Common.cmake")

    # Windows-based TAS plugins.
    # TASInput relies heavily on Windows APIs to work.
    # Note that we can't port config dialogs until we settle on a GUI frontend framework.
    add_subdirectory(Plugins.Win32.TASInput)

    # VIEW
    # ============================
    add_subdirectory(Views.Win32)
endif()

# QT6 VIEW
# ============================

if (MUPEN64RR_BUILD_QT6)
    # add_subdirectory(ViewModels.Qt6)
    add_subdirectory(Views.Qt6)
endif()

# TESTS
# ============================

add_subdirectory(Core.Tests)
add_subdirectory(Lua.TestLib)
