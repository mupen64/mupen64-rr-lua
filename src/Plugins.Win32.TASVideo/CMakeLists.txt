#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Win32.TASVideo MODULE
    "VI.hpp"
    "Types.hpp"
    "stdafx.h"
    "S2DEX2.hpp"
    "S2DEX.hpp"
    "RSP.hpp"
    "resource.h"
    "RDP.hpp"
    "OpenGL.hpp"
    "N64.hpp"
    "L3DEX2.hpp"
    "L3DEX.hpp"
    "L3D.hpp"
    "gSP.hpp"
    "glsl_combiner.hpp"
    "glN64.hpp"
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
    "stdafx.cpp"
    "S2DEX2.cpp"
    "S2DEX.cpp"
    "RSP.cpp"
    "RDP.cpp"
    "OpenGL.cpp"
    "N64.cpp"
    "L3DEX2.cpp"
    "L3DEX.cpp"
    "L3D.cpp"
    "gSP.cpp"
    "glsl_combiner.cpp"
    "glN64.cpp"
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

    "Resource.rc"
)
set_target_properties(Mupen64RR.Plugins.Win32.TASVideo PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "TASVideo"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(Mupen64RR.Plugins.Win32.TASVideo PRIVATE ".")
target_link_libraries(Mupen64RR.Plugins.Win32.TASVideo PRIVATE
    Mupen64RR.Plugins.Win32.Common
    vendor::hqx
    vendor::xbrz
    nlohmann_json::nlohmann_json
    GLEW::GLEW
    SDL3::SDL3
)
target_link_libraries(Mupen64RR.Plugins.Win32.TASVideo PRIVATE
    Comctl32
    uxtheme
    Msimg32
    Gdiplus
    opengl32
    glu32
    winmm
)
if (MUPEN64RR_LINK_STATIC)
    target_compile_definitions(Mupen64RR.Plugins.Win32.TASVideo PRIVATE
        "GLEW_STATIC"
    )
endif()
