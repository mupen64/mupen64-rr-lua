#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR_Plugins_TASVideo STATIC
    "Renderer.hpp"
    "Renderer.cpp"
    "Textures.hpp"
    "Common.hpp"
    "OpenGL.hpp"
    "glsl_combiner.hpp"
    "TASVideo.hpp"
    "FrameBuffer.hpp"
    "Config.hpp"
    "Combiner.hpp"
    "2xSAI.hpp"

    "Textures.cpp"
    "OpenGL.cpp"
    "glsl_combiner.cpp"
    "TASVideo.cpp"
    "FrameBuffer.cpp"
    "Config.cpp"
    "Combiner.cpp"
    "2xSAI.cpp"
)

mupen64rr_reuse_common_pch(Mupen64RR_Plugins_TASVideo)

if (MUPEN64RR_BUILD_WIN32)
    target_sources(Mupen64RR_Plugins_TASVideo PRIVATE
        "ConfigDialog_Win32.hpp"
        "Resource.h"

        "ConfigDialog_Win32.cpp"
        "TASVideo_Win32.cpp"
        "Resource.rc"
    )
    target_link_libraries(Mupen64RR_Plugins_TASVideo PRIVATE Mupen64RR_Plugins_Win32_Common)
else()
    target_link_libraries(Mupen64RR_Plugins_TASVideo PRIVATE Mupen64RR_Plugins_Common)
endif()

if (MUPEN64RR_BUILD_QT6)
    qt6_disable_unicode_defines(Mupen64RR_Plugins_TASVideo)
endif()

target_compile_features(Mupen64RR_Plugins_TASVideo PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Plugins_TASVideo PROPERTIES
    OUTPUT_NAME "TASVideo"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR_Plugins_TASVideo PRIVATE "." "../Plugins.TASRSP/Graphics")

if (MUPEN64RR_BUILD_WIN32)
    target_link_libraries(Mupen64RR_Plugins_TASVideo PRIVATE
        comctl32
        uxtheme
        msimg32
        opengl32
        spdlog::spdlog
    )
endif()

target_link_libraries(Mupen64RR_Plugins_TASVideo PRIVATE
    Mupen64RR_Plugins_TASRSP
    vendor::glad
    vendor::hqx
    vendor::xbrz
    nlohmann_json::nlohmann_json
    SDL3::SDL3
)

target_compile_definitions(Mupen64RR_Plugins_TASVideo PRIVATE
    PLUGIN_NAMESPACE=BuiltinTAS::TASVideo
    # g_plugin=M64RRBuiltinTASVideoPlugin
)
