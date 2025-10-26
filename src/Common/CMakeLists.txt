#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]
add_library(Mupen64RR.Common INTERFACE
    "include/CommonPCH.h"
    "include/IOUtils.h"
    "include/StrUtils.h"
    "include/MiscHelpers.h"
)
set_target_properties(Mupen64RR.Common PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(Mupen64RR.Common INTERFACE include)
target_link_libraries(Mupen64RR.Common INTERFACE
    libdeflate::libdeflate_$<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,static,shared>
    vendor::xxhash64
)
target_precompile_headers(Mupen64RR.Common INTERFACE "include/CommonPCH.h")