#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.TASVideo STATIC
    "VI.hpp"
    "Types.hpp"
    "Common.hpp"
    "S2DEX2.hpp"
    "S2DEX.hpp"
    "RSP.hpp"
    "RDP.hpp"
    "OpenGL.hpp"
    "N64.hpp"
    "L3DEX2.hpp"
    "L3DEX.hpp"
    "L3D.hpp"
    "gSP.hpp"
    "glsl_combiner.hpp"
    "TASVideo.hpp"
    "gDP.hpp"
    "GBI.hpp"
    "FrameBuffer.hpp"
    "F3DWRUS.hpp"
    "F3DPD.hpp"
    "F3DEX2.hpp"
    "F3DEX.hpp"
    "F3DDKR.hpp"
    "F3D.hpp"
    "DepthBuffer.hpp"
    "CRC.hpp"
    "convert.hpp"
    "Config.hpp"
    "Combiner.hpp"
    "3DMath.hpp"
    "2xSAI.hpp"

    "VI.cpp"
    "Textures.cpp"
    "S2DEX2.cpp"
    "S2DEX.cpp"
    "RSP.cpp"
    "RDP.cpp"
    "OpenGL.cpp"
    "L3DEX2.cpp"
    "L3DEX.cpp"
    "L3D.cpp"
    "gSP.cpp"
    "glsl_combiner.cpp"
    "TASVideo.cpp"
    "gDP.cpp"
    "GBI.cpp"
    "FrameBuffer.cpp"
    "F3DWRUS.cpp"
    "F3DPD.cpp"
    "F3DEX2.cpp"
    "F3DEX.cpp"
    "F3DDKR.cpp"
    "F3D.cpp"
    "DepthBuffer.cpp"
    "CRC.cpp"
    "Config.cpp"
    "Combiner.cpp"
    "2xSAI.cpp"
)

mupen64rr_reuse_common_pch(Mupen64RR.Plugins.TASVideo)

if (MUPEN64RR_BUILD_WIN32)
    target_sources(Mupen64RR.Plugins.TASVideo PRIVATE
        "MicrocodeDialog_Win32.hpp"
        "ConfigDialog_Win32.hpp"
        "Resource.h"

        "MicrocodeDialog_Win32.cpp"
        "ConfigDialog_Win32.cpp"
        "TASVideo_Win32.cpp"
        "Resource.rc"
    )
    target_link_libraries(Mupen64RR.Plugins.TASVideo PRIVATE Mupen64RR.Plugins.Win32.Common)
else()
    target_link_libraries(Mupen64RR.Plugins.TASVideo PRIVATE Mupen64RR.Plugins.Common)
endif()

target_compile_features(Mupen64RR.Plugins.TASVideo PRIVATE cxx_std_26)
target_compile_definitions(Mupen64RR.Plugins.TASVideo PRIVATE
    MUPEN64RR_BUILD_WIN32=$<BOOL:${MUPEN64RR_BUILD_WIN32}>
    MUPEN64RR_BUILD_QT6=$<BOOL:${MUPEN64RR_BUILD_QT6}>
)
set_target_properties(Mupen64RR.Plugins.TASVideo PROPERTIES
    OUTPUT_NAME "TASVideo"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.TASVideo PRIVATE ".")

if (MUPEN64RR_BUILD_WIN32)
    target_link_libraries(Mupen64RR.Plugins.TASVideo PRIVATE
        comctl32
        uxtheme
        msimg32
        opengl32
    )
endif()

target_link_libraries(Mupen64RR.Plugins.TASVideo PRIVATE
    vendor::glad
    vendor::hqx
    vendor::xbrz
    nlohmann_json::nlohmann_json
    SDL3::SDL3
)

target_compile_definitions(Mupen64RR.Plugins.TASVideo PRIVATE
    M64RRGetMetadata=M64RRBuiltinTASVideoGetMetadata
    M64RRProcessEvent=M64RRBuiltinTASVideoProcessEvent
    M64RRProcessDList=M64RRBuiltinTASVideoProcessDList
    M64RRReadVideo=M64RRBuiltinTASVideoReadVideo
    M64RRShowConfig=M64RRBuiltinTASVideoShowConfig
    g_plugin=M64RRBuiltinTASVideoPlugin
)
