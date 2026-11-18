#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

qt_add_library(Mupen64RR_Views_Qt6_Lua STATIC)
mupen64rr_reuse_common_pch(Mupen64RR_Views_Qt6_Lua)
qt_add_qml_module(Mupen64RR_Views_Qt6_Lua
    URI "Mupen64.Lua"
    IMPORTS
        TARGET Mupen64RR_Views_Qt6_Core
    SOURCES
        "LuaInstance.hpp"
        "LuaInstanceManager.hpp"
        "Resources.hpp"

        "LuaInstance.cpp"
        "LuaInstanceManager.cpp"
        "LuaInternal.cpp"
        "LuaInternal.hpp"
        "LuaGlobal.cpp"
        "LuaEmu.cpp"
        "LuaMemory.cpp"
        "Resources.cpp"
)
target_compile_features(Mupen64RR_Views_Qt6_Lua PUBLIC cxx_std_26)
target_include_directories(Mupen64RR_Views_Qt6_Lua PUBLIC ".")
target_link_libraries(Mupen64RR_Views_Qt6_Lua PUBLIC
    Mupen64RR_Common
    Mupen64RR_Common_Views
    Mupen64RR_Core
    Mupen64RR_Views_Qt6_Core

    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Lua::Lua
)

# add Lua resources; disable compression to allow zero-copy loading at runtime
set(lua_dir "${PROJECT_SOURCE_DIR}/src/Lua")
set(lua_inspect "${PROJECT_SOURCE_DIR}/vendor/lua-modules/inspect.lua")
set_source_files_properties("${lua_inspect}" PROPERTIES QT_RESOURCE_ALIAS "inspect.lua")
qt_add_resources(Mupen64RR_Views_Qt6_Lua "Mupen64RR_Views_Qt6_Lua_Scripts"
    PREFIX "/lua"
    BASE "${lua_dir}"
    FILES "${lua_dir}/sandbox.lua" "${lua_dir}/api.lua" "${lua_inspect}"
    OPTIONS "-no-compress"
)
