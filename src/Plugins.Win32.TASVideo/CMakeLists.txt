#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Plugins.Win32.TASVideo MODULE
    "VI.h"
    "Types.h"
    "texture_env.h"
    "texture_env_combine.h"
    "stdafx.h"
    "S2DEX2.h"
    "S2DEX.h"
    "RSP.h"
    "resource.h"
    "RDP.h"
    "OpenGL.h"
    "NV_register_combiners.h"
    "N64.h"
    "L3DEX2.h"
    "L3DEX.h"
    "L3D.h"
    "gSP.h"
    "glN64.h"
    "gDP.h"
    "GBI.h"
    "FrameBuffer.h"
    "F3DWRUS.h"
    "F3DPD.h"
    "F3DEX2.h"
    "F3DEX.h"
    "F3DDKR.h"
    "F3D.h"
    "DepthBuffer.h"
    "Debug.h"
    "CRC.h"
    "convert.h"
    "Config.h"
    "Combiner.h"
    "3DMath.h"
    "2xSAI.h"

    "VI.cpp"
    "Textures.cpp"
    "texture_env.cpp"
    "texture_env_combine.cpp"
    "stdafx.cpp"
    "S2DEX2.cpp"
    "S2DEX.cpp"
    "RSP.cpp"
    "RDP.cpp"
    "OpenGL.cpp"
    "NV_register_combiners.cpp"
    "N64.cpp"
    "L3DEX2.cpp"
    "L3DEX.cpp"
    "L3D.cpp"
    "gSP.cpp"
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
    "Debug.cpp"
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
    GLEW::GLEW
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
target_compile_definitions(Mupen64RR.Plugins.Win32.TASVideo PRIVATE 
    "GLEW_STATIC"
)