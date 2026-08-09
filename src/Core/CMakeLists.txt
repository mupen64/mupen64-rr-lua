#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Core.Headers INTERFACE
    "include/m64rr/API.hpp"
    "include/m64rr/Plugin.hpp"
    "include/m64rr/Types.hpp"
)
target_compile_features(Mupen64RR.Core.Headers INTERFACE cxx_std_26)
target_include_directories(Mupen64RR.Core.Headers INTERFACE "include")
target_link_libraries(Mupen64RR.Core.Headers INTERFACE Mupen64RR.Common)

add_library(Mupen64RR.Core STATIC
    "Core.hpp"
    "Alloc.hpp"
    "Cheats.hpp"
    "Memory/PifLut.hpp"
    "Memory/DMA.hpp"
    "Memory/FlashRAM.hpp"
    "Memory/Memory.hpp"
    "Memory/Pif.hpp"
    "Memory/Savestates.hpp"
    "Memory/ParityChecker.hpp"
    "Memory/Summercart.hpp"
    "Memory/TLB.hpp"
    "R4300/Debug.hpp"
    "R4300/Ops.hpp"
    "R4300/Cop1Helpers.hpp"
    "R4300/Disasm.hpp"
    "R4300/Exception.hpp"
    "R4300/Interrupt.hpp"
    "R4300/Macros.hpp"
    "R4300/R4300.hpp"
    "R4300/Recomp.hpp"
    "R4300/Recomph.hpp"
    "R4300/Rom.hpp"
    "R4300/Timers.hpp"
    "R4300/Tracelog.hpp"
    "R4300/VCR.hpp"

    "Core.cpp"
    "Alloc.cpp"
    "Cheats.cpp"
    "Memory/PifLut.cpp"
    "Memory/DMA.cpp"
    "Memory/FlashRAM.cpp"
    "Memory/Memory.cpp"
    "Memory/Pif.cpp"
    "Memory/Savestates.cpp"
    "Memory/ParityChecker.cpp"
    "Memory/Summercart.cpp"
    "Memory/TLB.cpp"
    "R4300/Debug.cpp"
    "R4300/PureInterp.cpp"
    "R4300/Cop0.cpp"
    "R4300/Cop1.cpp"
    "R4300/Cop1D.cpp"
    "R4300/Cop1Helpers.cpp"
    "R4300/Cop1L.cpp"
    "R4300/Cop1S.cpp"
    "R4300/Cop1W.cpp"
    "R4300/Disasm.cpp"
    "R4300/Exception.cpp"
    "R4300/Interrupt.cpp"
    "R4300/R4300.cpp"
    "R4300/Recomp.cpp"
    "R4300/RegImm.cpp"
    "R4300/Rom.cpp"
    "R4300/Special.cpp"
    "R4300/Timers.cpp"
    "R4300/Tracelog.cpp"
    "R4300/BC.cpp"

    "R4300/VCR.cpp"
)
target_compile_features(Mupen64RR.Core PUBLIC cxx_std_26)
set_target_properties(Mupen64RR.Core PROPERTIES
    OUTPUT_NAME Core
)
target_include_directories(Mupen64RR.Core PUBLIC "include")
target_include_directories(Mupen64RR.Core PRIVATE ".")
target_link_libraries(Mupen64RR.Core PRIVATE
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    PkgConfig::lz4
    vendor::aladdin-md5
)
target_link_libraries(Mupen64RR.Core PUBLIC
    Mupen64RR.Common
    Mupen64RR.Core.Headers
)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Mupen64RR.Core PRIVATE -Werror=nonportable-include-path)
endif()

# options: disable dynarec
if (${MUPEN64RR_ENABLE_DYNAREC})
    target_compile_definitions(Mupen64RR.Core PRIVATE "MUPEN64RR_ENABLE_DYNAREC")
    # Select the dynarec backend by target architecture.
    #   x86-64 -> R4300/x86_64 (x86-64 recompiler)
    #   x86    -> R4300/x86 (original x86-32 recompiler)
    if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(AMD64|x86_64)$")
        set(DYNAREC_DIR "R4300/x86_64")
    elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86|i[346]86)$")
        set(DYNAREC_DIR "R4300/x86")
    else()
        message(FATAL_ERROR "Dynarec is not supported on this platform!")
    endif()

    target_sources(Mupen64RR.Core PRIVATE
        "${DYNAREC_DIR}/Assemble.hpp"
        "${DYNAREC_DIR}/Gcop1Helpers.hpp"
        "${DYNAREC_DIR}/RegCache.hpp"
        "${DYNAREC_DIR}/Assemble.cpp"
        "${DYNAREC_DIR}/GBc.cpp"
        "${DYNAREC_DIR}/GCop0.cpp"
        "${DYNAREC_DIR}/GCop1.cpp"
        "${DYNAREC_DIR}/GCop1D.cpp"
        "${DYNAREC_DIR}/GCop1Helpers.cpp"
        "${DYNAREC_DIR}/GCop1L.cpp"
        "${DYNAREC_DIR}/GCop1S.cpp"
        "${DYNAREC_DIR}/GCop1W.cpp"
        "${DYNAREC_DIR}/GR4300.cpp"
        "${DYNAREC_DIR}/GRegImm.cpp"
        "${DYNAREC_DIR}/GSpecial.cpp"
        "${DYNAREC_DIR}/GTLB.cpp"
        "${DYNAREC_DIR}/RegCache.cpp"
        "${DYNAREC_DIR}/RJump.cpp"
    )
endif()

# FOR TESTING ONLY. DO NOT USE OTHERWISE.
add_library(Mupen64RR.Core._TestIncludes INTERFACE)
target_link_libraries(Mupen64RR.Core._TestIncludes INTERFACE
    Mupen64RR.Core
)
target_include_directories(Mupen64RR.Core._TestIncludes INTERFACE "../" "./")
