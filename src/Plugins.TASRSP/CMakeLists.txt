#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR_Plugins_TASRSP STATIC
    "Config.hpp"
    "HLE.hpp"
    "Main.hpp"

    "Config.cpp"
    "JPEG.cpp"
    "Main.cpp"
    "MP3.cpp"
    "UCode1.cpp"
    "UCode2.cpp"
    "UCode3.cpp"

    "Graphics/Common.hpp"
    "Graphics/RenderState.hpp"
    "include/BuiltinTAS/GraphicsTask.hpp"
    "Graphics/DrawList.hpp"
    "Graphics/DrawList.cpp"
    "Graphics/3DMath.hpp"
    "Graphics/CRC.cpp"
    "Graphics/CRC.hpp"
    "Graphics/DepthBuffer.cpp"
    "Graphics/DepthBuffer.hpp"
    "Graphics/F3D.cpp"
    "Graphics/F3D.hpp"
    "Graphics/F3DDKR.cpp"
    "Graphics/F3DDKR.hpp"
    "Graphics/F3DEX.cpp"
    "Graphics/F3DEX.hpp"
    "Graphics/F3DEX2.cpp"
    "Graphics/F3DEX2.hpp"
    "Graphics/F3DEX3.cpp"
    "Graphics/F3DEX3.hpp"
    "Graphics/F3DPD.cpp"
    "Graphics/F3DPD.hpp"
    "Graphics/F3DWRUS.cpp"
    "Graphics/F3DWRUS.hpp"
    "Graphics/GBI.cpp"
    "Graphics/GBI.hpp"
    "Graphics/L3D.cpp"
    "Graphics/L3D.hpp"
    "Graphics/L3DEX.cpp"
    "Graphics/L3DEX.hpp"
    "Graphics/L3DEX2.cpp"
    "Graphics/L3DEX2.hpp"
    "Graphics/N64.hpp"
    "Graphics/RDP.cpp"
    "Graphics/RDP.hpp"
    "Graphics/RDPConstants.hpp"
    "Graphics/RSP.cpp"
    "Graphics/RSP.hpp"
    "Graphics/S2DEX.cpp"
    "Graphics/S2DEX.hpp"
    "Graphics/S2DEX2.cpp"
    "Graphics/S2DEX2.hpp"
    "Graphics/Types.hpp"
    "Graphics/convert.hpp"
    "Graphics/gDP.cpp"
    "Graphics/gDP.hpp"
    "Graphics/gSP.cpp"
    "Graphics/gSP.hpp"
)

if (MUPEN64RR_BUILD_WIN32)
    target_sources(Mupen64RR_Plugins_TASRSP PRIVATE
        "Graphics/MicrocodeDialog_Win32.cpp"
        "Graphics/MicrocodeDialog_Win32.hpp"
    )
endif()

target_compile_features(Mupen64RR_Plugins_TASRSP PRIVATE cxx_std_26)
set_target_properties(Mupen64RR_Plugins_TASRSP PROPERTIES
    OUTPUT_NAME "TASRSP"
    LIBRARY_OUTPUT_DIRECTORY "${MUPEN64RR_PLUGIN_OUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
mupen64rr_reuse_common_pch(Mupen64RR_Plugins_TASRSP)
target_include_directories(Mupen64RR_Plugins_TASRSP PUBLIC "include" PRIVATE ".")
target_link_libraries(Mupen64RR_Plugins_TASRSP PRIVATE
    Mupen64RR_Plugins_Common
    nlohmann_json::nlohmann_json
)

if (MUPEN64RR_BUILD_QT6)
    qt6_disable_unicode_defines(Mupen64RR_Plugins_TASRSP)
endif()


target_compile_definitions(Mupen64RR_Plugins_TASRSP PRIVATE
    PLUGIN_NAMESPACE=BuiltinTAS::TASRSP
    # g_plugin=M64RRBuiltinTASRSPPlugin
)
