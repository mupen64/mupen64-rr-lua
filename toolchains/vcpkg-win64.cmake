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

# setup a few last values for vcpkg
set(VCPKG_TARGET_TRIPLET "${_vs_target_arch}-windows-static" CACHE INTERNAL "target triplet for vcpkg")
message(STATUS "VS architecture set to: ${_vs_target_arch}")

# hand off the rest to vcpkg
include(${MUPEN64RR_VCPKG_TOOLCHAIN})