#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Views.Win32.Headers INTERFACE
    "include/Views.Win32/ZESpec.h"
)
set_target_properties(Mupen64RR.Views.Win32.Headers PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(Mupen64RR.Views.Win32.Headers INTERFACE "include")

add_executable(Mupen64RR.Views.Win32 WIN32
    # headers
    "stdafx.h"
    "ViewHelpers.hpp"
    "ThreadPool.hpp"
    "Hotkey.hpp"
    "capture/encoders/VFWEncoder.hpp"
    "capture/encoders/Encoder.hpp"
    "capture/encoders/FFmpegEncoder.hpp"
    "capture/CaptureManager.hpp"
    "capture/Resampler.hpp"
    "DialogService.hpp"
    "components/CoreUtils.hpp"
    "components/Validators.hpp"
    "components/PianoRoll.hpp"
    "components/RecentItems.hpp"
    "components/UpdateChecker.hpp"
    "Loggers.hpp"
    "components/Cheats.hpp"
    "components/ConfigDialog.hpp"
    "components/CrashManager.hpp"
    "components/Dispatcher.hpp"
    "components/MGECompositor.hpp"
    "components/MovieDialog.hpp"
    "components/RomBrowser.hpp"
    "components/Seeker.hpp"
    "components/Statusbar.hpp"
    "components/FilePicker.hpp"
    "components/CLI.hpp"
    "components/SettingsListView.hpp"
    "components/HotkeyTracker.hpp"
    "components/CommandPalette.hpp"
    "components/ParameterPalette.hpp"
    "components/TextEditDialog.hpp"
    "components/ReorderableListView.hpp"
    "action/ActionManager.hpp"
    "action/ActionMenu.hpp"
    "action/AppActions.hpp"
    "Main.hpp"
    "Config.hpp"
    "lua/LuaHelpers.hpp"
    "lua/LuaTypes.hpp"
    "lua/LuaManager.hpp"
    "lua/LuaCallbacks.hpp"
    "lua/LuaRegistry.hpp"
    "lua/LuaRenderer.hpp"
    "lua/LuaDialog.hpp"
    "lua/modules/AVI.hpp"
    "lua/modules/D2D.hpp"
    "lua/modules/Emu.hpp"
    "lua/modules/Global.hpp"
    "lua/modules/Input.hpp"
    "lua/modules/IOHelper.hpp"
    "lua/modules/Joypad.hpp"
    "lua/modules/Memory.hpp"
    "lua/modules/Movie.hpp"
    "lua/modules/Savestate.hpp"
    "lua/modules/WGUI.hpp"
    "lua/modules/Action.hpp"
    "lua/modules/Hotkey.hpp"
    "lua/modules/Clipboard.hpp"
    "lua/presenters/DCompPresenter.hpp"
    "lua/presenters/GDIPresenter.hpp"
    "lua/presenters/Presenter.hpp"
    "Messenger.hpp"
    "plugin/Plugin.hpp"
    "plugin/ZEPlugin.hpp"
    "plugin/M64RRPlugin.hpp"
    "resource.h"
    "ResizeAnchor.hpp"

    # sources
    "ThreadPool.cpp"
    "capture/encoders/VFWEncoder.cpp"
    "capture/encoders/FFmpegEncoder.cpp"
    "capture/CaptureManager.cpp"
    "capture/Resampler.cpp"
    "Config.cpp"
    "Hotkey.cpp"
    "DialogService.cpp"
    "components/CoreUtils.cpp"
    "components/Validators.cpp"
    "components/Cheats.cpp"
    "components/ConfigDialog.cpp"
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
    "components/HotkeyTracker.cpp"
    "components/CommandPalette.cpp"
    "components/ParameterPalette.cpp"
    "components/TextEditDialog.cpp"
    "components/ReorderableListView.cpp"
    "Loggers.cpp"
    "action/ActionManager.cpp"
    "action/ActionMenu.cpp"
    "action/AppActions.cpp"
    "Main.cpp"
    "lua/LuaHelpers.cpp"
    "lua/LuaManager.cpp"
    "lua/LuaCallbacks.cpp"
    "lua/LuaRegistry.cpp"
    "lua/LuaRenderer.cpp"
    "lua/LuaDialog.cpp"
    "lua/presenters/DCompPresenter.cpp"
    "lua/presenters/GDIPresenter.cpp"
    "Messenger.cpp"
    "plugin/Plugin.cpp"
    "plugin/ZEPlugin.cpp"
    "plugin/M64RRPlugin.cpp"
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
mupen64rr_copy_vcpkg_deps(Mupen64RR.Views.Win32)
# Dependencies of the resource file. These are specified manually,
# but it wouldn't be impossible to write a scanner to gather this info automagically.
set_source_files_properties(
    SOURCES "Resource.rc" PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/app.manifest"
    "${CMAKE_CURRENT_SOURCE_DIR}/Lua/api.lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/Lua/shims.lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/Lua/sandbox.lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/lua-modules/inspect.lua"
)
target_precompile_headers(Mupen64RR.Views.Win32 PRIVATE "stdafx.h")
target_include_directories(Mupen64RR.Views.Win32 PRIVATE ".")
target_link_libraries(Mupen64RR.Views.Win32 PRIVATE
    Mupen64RR.Common
    Mupen64RR.Common.Win32
    Mupen64RR.Core
    Mupen64RR.Views.Win32.Headers
    Lua::Lua
    spdlog::spdlog
    SDL3::SDL3
    nlohmann_json::nlohmann_json
    PkgConfig::speexdsp
    PkgConfig::libav
    vendor::argh
    vendor::bs-thread-pool
    vendor::windarkmode
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
    dxguid
    Dwmapi
    d3dcompiler
    Winhttp
)

target_compile_definitions(Mupen64RR.Views.Win32 PRIVATE
    "MINI_CASE_SENSITIVE"
    # "SPDLOG_WCHAR_TO_UTF8_SUPPORT"
    "VERSION_SUFFIX=L\"${MUPEN64RR_VERSION_SUFFIX}\""
    "$<$<BOOL:${MUPEN64RR_NIGHTLY}>:NIGHTLY>"
)

target_link_options(Mupen64RR.Views.Win32 PRIVATE "/MANIFEST:NO")
