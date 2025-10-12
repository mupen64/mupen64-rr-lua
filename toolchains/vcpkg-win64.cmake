#[===[
Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).

SPDX-License-Identifier: GPL-2.0-or-later
]===]

# Toolchain file for the Visual Studio pipeline.
# =========================================================

if(NOT ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  message(FATAL_ERROR "This toolchain file is designed for use on Windows.")
endif()

if(NOT DEFINED ENV{VCINSTALLDIR})
  message(FATAL_ERROR "Please build from a Visual Studio developer environment (cmd.exe or PowerShell)")
endif()

# OPTIONS
# =========================================================
set(_toolchain_sanitizer_keys OFF ASAN)
set(_toolchain_sanitizer_values "" "-asan")

set(MUPEN64RR_USE_SANITIZER OFF CACHE STRING "Specifies a sanitizer to compile with. [${_toolchain_sanitizer_keys}]")

# validate sanitizer option
list(FIND _toolchain_sanitizer_keys "${MUPEN64RR_USE_SANITIZER}" _key_index)
if ("${_key_index}" LESS 0)
  message(FATAL_ERROR "expected MUPEN64RR_USE_SANITIZER to be one of [${_toolchain_sanitizer_keys}]")
endif()
list(GET _toolchain_sanitizer_values "${_key_index}" _vcpkg_san_suffix)

# CONFIGURATION
# =========================================================

# determine host and target architectures based on folder structure of MSVC.
# the path ends with Host<host_arch>/<target_arch>/cl.exe for a given host/target arch.
find_program(_cl_exe_path cl.exe NO_CACHE)
message(STATUS "Found cl.exe at ${_cl_exe_path}")

if("${_cl_exe_path}" MATCHES [=[/Host(.+)/(.+)/cl\.exe$]=])
  set(_vs_host_arch "${CMAKE_MATCH_1}")
  set(_vs_target_arch "${CMAKE_MATCH_2}")
else()
  message(FATAL_ERROR "Failed to determine host/target architectures!")
endif()

# Find the toolchain file using the current environment
if(NOT DEFINED CACHE{MUPEN64RR_VCPKG_TOOLCHAIN})
  set(
    MUPEN64RR_VCPKG_TOOLCHAIN "$ENV{VCINSTALLDIR}\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake"
    CACHE INTERNAL "Location of vcpkg's toolchain file."
  )
endif()

if(NOT EXISTS "${MUPEN64RR_VCPKG_TOOLCHAIN}")
  message(FATAL_ERROR "Expected vcpkg.cmake at ${MUPEN64RR_VCPKG_TOOLCHAIN}")
endif()

# set some necessary settings to get compilation to work properly
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "MSVCRT variant needed to get things to work.")

set(CMAKE_C_FLAGS_DEBUG_INIT)
set(CMAKE_CXX_FLAGS_DEBUG_INIT)

if ("${MUPEN64RR_USE_SANITIZER}" STREQUAL "ASAN")
  list(APPEND CMAKE_C_FLAGS_DEBUG_INIT "/fsanitize=address")
  list(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT "/fsanitize=address")
endif()

# setup a few last values for vcpkg
set(VCPKG_TARGET_TRIPLET "${_vs_target_arch}-windows-static${_vcpkg_san_suffix}" CACHE INTERNAL "target triplet for vcpkg")
set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/vcpkg-triplets")
message(STATUS "VS architecture set to: ${_vs_target_arch}")



# hand off the rest to vcpkg
include(${MUPEN64RR_VCPKG_TOOLCHAIN})