#[===[
Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

add_subdirectory(aladdin-md5)
add_library(vendor::aladdin-md5 ALIAS aladdin-md5)

add_subdirectory(argh)
add_library(vendor::argh ALIAS argh)

add_subdirectory(bs-thread-pool)
add_library(vendor::bs-thread-pool ALIAS bs-thread-pool)

add_subdirectory(microlru)
add_library(vendor::microlru ALIAS microlru)

add_subdirectory(mini)
add_library(vendor::mini ALIAS mini)

add_subdirectory(xxhash64)
add_library(vendor::xxhash64 ALIAS xxhash64)

add_subdirectory(hqx)
add_library(vendor::hqx ALIAS hqx)

add_subdirectory(xbrz)
add_library(vendor::xbrz ALIAS xbrz)