#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Views.Win32.Headers INTERFACE
    "include/Views.Win32/ViewPlugin.h"
)
set_target_properties(Mupen64RR.Views.Win32.Headers PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(Mupen64RR.Views.Win32.Headers INTERFACE "include")

add_executable(Mupen64RR.Views.Win32 WIN32
    # headers
    "stdafx.h"
    "ViewHelpers.h"
    "ThreadPool.h"
    "ActionManager.h"
    "Hotkey.h"
    "capture/encoders/VFWEncoder.h"
    "capture/encoders/Encoder.h"
    "capture/encoders/FFmpegEncoder.h"
    "capture/EncodingManager.h"
    "capture/Resampler.h"
    "DialogService.h"
    "components/Benchmark.h"
    "components/PianoRoll.h"
    "components/RecentItems.h"
    "components/UpdateChecker.h"
    "components/Compare.h"
    "Loggers.h"
    "components/Cheats.h"
    "components/ConfigDialog.h"
    "components/CoreDbg.h"
    "components/CrashManager.h"
    "components/Dispatcher.h"
    "components/MGECompositor.h"
    "components/MovieDialog.h"
    "components/RomBrowser.h"
    "components/Seeker.h"
    "components/Statusbar.h"
    "components/FilePicker.h"
    "components/CLI.h"
    "components/SettingsListView.h"
    "components/ActionMenu.h"
    "components/HotkeyTracker.h"
    "components/AppActions.h"
    "components/LuaDialog.h"
    "components/CommandPalette.h"
    "components/TextEditDialog.h"
    "Main.h"
    "Config.h"
    "lua/LuaTypes.h"
    "lua/LuaManager.h"
    "lua/LuaCallbacks.h"
    "lua/LuaRegistry.h"
    "lua/LuaRenderer.h"
    "lua/modules/AVI.h"
    "lua/modules/D2D.h"
    "lua/modules/Emu.h"
    "lua/modules/Global.h"
    "lua/modules/Input.h"
    "lua/modules/IOHelper.h"
    "lua/modules/Joypad.h"
    "lua/modules/Memory.h"
    "lua/modules/Movie.h"
    "lua/modules/Savestate.h"
    "lua/modules/WGUI.h"
    "lua/modules/Action.h"
    "lua/modules/Hotkey.h"
    "lua/modules/Clipboard.h"
    "lua/presenters/DCompPresenter.h"
    "lua/presenters/GDIPresenter.h"
    "lua/presenters/Presenter.h"
    "Messenger.h"
    "Plugin.h"
    "resource.h"
    "ResizeAnchor.h"

    # sources
    "ThreadPool.cpp"
    "capture/encoders/VFWEncoder.cpp"
    "capture/encoders/FFmpegEncoder.cpp"
    "capture/EncodingManager.cpp"
    "capture/Resampler.cpp"
    "Config.cpp"
    "ActionManager.cpp"
    "Hotkey.cpp"
    "DialogService.cpp"
    "components/Compare.cpp"
    "components/Benchmark.cpp"
    "components/Cheats.cpp"
    "components/ConfigDialog.cpp"
    "components/CoreDbg.cpp"
    "components/CrashManager.cpp"
    "components/Dispatcher.cpp"
    "components/MGECompositor.cpp"
    "components/MovieDialog.cpp"
    "components/PianoRoll.cpp"
    "components/RecentItems.cpp"
    "components/RomBrowser.cpp"
    "components/Seeker.cpp"
    "components/Statusbar.cpp"
    "components/UpdateChecker.cpp"
    "components/FilePicker.cpp"
    "components/CLI.cpp"
    "components/SettingsListView.cpp"
    "components/ActionMenu.cpp"
    "components/HotkeyTracker.cpp"
    "components/AppActions.cpp"
    "components/LuaDialog.cpp"
    "components/CommandPalette.cpp"
    "components/TextEditDialog.cpp"
    "Loggers.cpp"
    "Main.cpp"
    "lua/LuaManager.cpp"
    "lua/LuaCallbacks.cpp"
    "lua/LuaRegistry.cpp"
    "lua/LuaRenderer.cpp"
    "lua/presenters/DCompPresenter.cpp"
    "lua/presenters/GDIPresenter.cpp"
    "Messenger.cpp"
    "Plugin.cpp"
    "ResizeAnchor.cpp"

    # Resource script
    "Resource.rc"
)
set_target_properties(Mupen64RR.Views.Win32 PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    # output location
    OUTPUT_NAME "mupen64"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
# Dependencies of the resource file. These are specified manually,
# but it wouldn't be impossible to write a scanner to gather this info automagically.
set_source_files_properties(
    SOURCES "Resource.rc" PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/app.manifest"
    "${CMAKE_CURRENT_SOURCE_DIR}/../api.lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/../shims.lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sandbox.lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/lua-modules/inspect.lua"
)
target_precompile_headers(Mupen64RR.Views.Win32 PRIVATE "stdafx.h")
target_include_directories(Mupen64RR.Views.Win32 PRIVATE ".")
target_link_libraries(Mupen64RR.Views.Win32 PRIVATE
    Mupen64RR.Common
    Mupen64RR.Core
    Mupen64RR.Views.Win32.Headers
    Lua::Lua
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    PkgConfig::speexdsp
    vendor::argh
    vendor::bs-thread-pool
    vendor::microlru
    vendor::mini
)
# Windows library linkages
target_link_libraries(Mupen64RR.Views.Win32 PRIVATE
    Shlwapi
    vfw32
    winmm
    propsys
    Comctl32
    uxtheme
    Msimg32
    Gdiplus
    d2d1
    dwrite
    Dbghelp
    Dcomp
    D3D11
    DXGI
    d3dcompiler
    Winhttp
)

target_compile_definitions(Mupen64RR.Views.Win32 PRIVATE 
    "MINI_CASE_SENSITIVE"
    # "SPDLOG_WCHAR_TO_UTF8_SUPPORT"
    "VERSION_SUFFIX=L\"${MUPEN64RR_VERSION_SUFFIX}\""
)