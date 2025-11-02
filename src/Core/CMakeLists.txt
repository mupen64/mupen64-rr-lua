#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_library(Mupen64RR.Core.Headers INTERFACE
    "include/core_plugin.h"
    "include/core_types.h"
    "include/core_api.h"
)
set_target_properties(Mupen64RR.Core.Headers PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(Mupen64RR.Core.Headers INTERFACE "include")
target_link_libraries(Mupen64RR.Core.Headers INTERFACE Mupen64RR.Common)

add_library(Mupen64RR.Core STATIC
    "Core.h"
    "alloc.h"
    "cheats.h"
    "memory/pif_lut.h"
    "memory/dma.h"
    "memory/flashram.h"
    "memory/memory.h"
    "memory/pif.h"
    "memory/savestates.h"
    "memory/summercart.h"
    "memory/tlb.h"
    "r4300/debugger.h"
    "r4300/ops.h"
    "r4300/cop1_helpers.h"
    "r4300/disasm.h"
    "r4300/exception.h"
    "r4300/interrupt.h"
    "r4300/macros.h"
    "r4300/r4300.h"
    "r4300/recomp.h"
    "r4300/recomph.h"
    "r4300/rom.h"
    "r4300/timers.h"
    "r4300/tracelog.h"
    "r4300/vcr.h"
    "r4300/x86/assemble.h"
    "r4300/x86/gcop1_helpers.h"
    "r4300/x86/regcache.h"

    "Core.cpp"
    "alloc.cpp"
    "cheats.cpp"
    "memory/pif_lut.cpp"
    "memory/dma.cpp"
    "memory/flashram.cpp"
    "memory/memory.cpp"
    "memory/pif.cpp"
    "memory/savestates.cpp"
    "memory/summercart.cpp"
    "memory/tlb.cpp"
    "r4300/debugger.cpp"
    "r4300/pure_interp.cpp"
    "r4300/cop0.cpp"
    "r4300/cop1.cpp"
    "r4300/cop1_d.cpp"
    "r4300/cop1_helpers.cpp"
    "r4300/cop1_l.cpp"
    "r4300/cop1_s.cpp"
    "r4300/cop1_w.cpp"
    "r4300/disasm.cpp"
    "r4300/exception.cpp"
    "r4300/interrupt.cpp"
    "r4300/r4300.cpp"
    "r4300/recomp.cpp"
    "r4300/regimm.cpp"
    "r4300/rom.cpp"
    "r4300/special.cpp"
    "r4300/timers.cpp"
    "r4300/tracelog.cpp"
    "r4300/bc.cpp"
    
    "r4300/vcr.cpp"
)
set_target_properties(Mupen64RR.Core PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME Core
)
target_include_directories(Mupen64RR.Core PUBLIC "include")
target_include_directories(Mupen64RR.Core PRIVATE ".")
target_link_libraries(Mupen64RR.Core PRIVATE
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    vendor::aladdin-md5
)
target_link_libraries(Mupen64RR.Core PUBLIC
    Mupen64RR.Common
    Mupen64RR.Core.Headers
)

# options: disable dynarec
if (${MUPEN64RR_ENABLE_DYNAREC})
    target_compile_definitions(Mupen64RR.Core PRIVATE "MUPEN64RR_ENABLE_DYNAREC")
    target_sources(Mupen64RR.Core PRIVATE 
        "r4300/x86/assemble.cpp"
        "r4300/x86/gbc.cpp"
        "r4300/x86/gcop0.cpp"
        "r4300/x86/gcop1.cpp"
        "r4300/x86/gcop1_d.cpp"
        "r4300/x86/gcop1_helpers.cpp"
        "r4300/x86/gcop1_l.cpp"
        "r4300/x86/gcop1_s.cpp"
        "r4300/x86/gcop1_w.cpp"
        "r4300/x86/gr4300.cpp"
        "r4300/x86/gregimm.cpp"
        "r4300/x86/gspecial.cpp"
        "r4300/x86/gtlb.cpp"
        "r4300/x86/regcache.cpp"
        "r4300/x86/rjump.cpp"
    )
endif()

# FOR TESTING ONLY. DO NOT USE OTHERWISE.
add_library(Mupen64RR.Core._TestIncludes INTERFACE)
target_link_libraries(Mupen64RR.Core._TestIncludes INTERFACE
    Mupen64RR.Core
)
target_include_directories(Mupen64RR.Core._TestIncludes INTERFACE "../" "./")