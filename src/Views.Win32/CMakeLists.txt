#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Views.Win32.Headers INTERFACE
    "include/Views.Win32/ZESpec.h"
)
target_compile_features(Mupen64RR.Views.Win32.Headers INTERFACE cxx_std_23)
target_include_directories(Mupen64RR.Views.Win32.Headers INTERFACE "include")

add_executable(Mupen64RR.Views.Win32 WIN32
    # headers
    "Common.hpp"
    "ThreadPool.hpp"
    "HotkeyUtils.hpp"
    "CaptureManager.hpp"
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
    "components/HotkeyTracker.hpp"
    "components/CommandPalette.hpp"
    "components/ParameterPalette.hpp"
    "components/TextEditDialog.hpp"
    "components/ReorderableListView.hpp"
    "action/ActionMenu.hpp"
    "action/AppActions.hpp"
    "Main.hpp"
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
    "plugin/Plugin.hpp"
    "plugin/ZEPlugin.hpp"
    "plugin/M64RRPlugin.hpp"
    "resource.h"

    # sources
    "ThreadPool.cpp"
    "CaptureManager.cpp"
    "HotkeyUtils.cpp"
    "WinDialogService.cpp"
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
    "components/HotkeyTracker.cpp"
    "components/CommandPalette.cpp"
    "components/ParameterPalette.cpp"
    "components/TextEditDialog.cpp"
    "components/ReorderableListView.cpp"
    "Loggers.cpp"
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
    "plugin/Plugin.cpp"
    "plugin/ZEPlugin.cpp"
    "plugin/M64RRPlugin.cpp"

    # Resource script
    "Resource.rc"
)
target_compile_features(Mupen64RR.Views.Win32 PRIVATE cxx_std_23)
set_target_properties(Mupen64RR.Views.Win32 PROPERTIES
    # output location
    OUTPUT_NAME "mupen64"
    RUNTIME_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${MUPEN64RR_OUT_DIR}"
)
mupen64rr_copy_vcpkg_deps(Mupen64RR.Views.Win32)
mupen64rr_copy_mingw_runtime(Mupen64RR.Views.Win32)
add_custom_command(TARGET Mupen64RR.Views.Win32 POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        -D "MUPEN64RR_OUT_DIR=${MUPEN64RR_OUT_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RenameSpeexDsp.cmake"
    VERBATIM
)
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
target_include_directories(Mupen64RR.Views.Win32 PRIVATE ".")
target_link_libraries(Mupen64RR.Views.Win32 PRIVATE
    Mupen64RR.Common
    Mupen64RR.Common.Views
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
    shlwapi
    vfw32
    winmm
    propsys
    comctl32
    uxtheme
    msimg32
    gdiplus
    d2d1
    dwrite
    dbghelp
    dcomp
    d3d11
    dxgi
    dxguid
    dwmapi
    d3dcompiler
    winhttp
)

target_compile_definitions(Mupen64RR.Views.Win32 PRIVATE
    "MINI_CASE_SENSITIVE"
    # "SPDLOG_WCHAR_TO_UTF8_SUPPORT"
    "VERSION_SUFFIX=L\"${MUPEN64RR_VERSION_SUFFIX}\""
    "$<$<BOOL:${MUPEN64RR_NIGHTLY}>:NIGHTLY>"
)

if(MSVC)
    target_link_options(Mupen64RR.Views.Win32 PRIVATE "/MANIFEST:NO")
endif()
