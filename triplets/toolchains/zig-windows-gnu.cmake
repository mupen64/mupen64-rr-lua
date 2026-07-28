# Use Zig's Clang driver and bundled libc++ so vcpkg C++ libraries match
# applications built by `zig build -Dtarget=*-windows-gnu`.
set(CMAKE_SYSTEM_NAME Windows)

# Forward the vcpkg architecture to nested CMake try_compile projects.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES VCPKG_TARGET_ARCHITECTURE)

# VCPKG_TARGET_ARCHITECTURE is not forwarded to CMake's nested try_compile
# projects unless it is listed above.
if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i686")
    set(CMAKE_SYSTEM_PROCESSOR i686)
else()
    message(FATAL_ERROR "Unsupported Zig Windows architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

find_program(ZIG_EXECUTABLE NAMES zig REQUIRED)
find_program(CMAKE_RC_COMPILER NAMES ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32-windres windres REQUIRED)

set(CMAKE_C_COMPILER "${ZIG_EXECUTABLE}")
set(CMAKE_C_COMPILER_ARG1 cc)
set(CMAKE_CXX_COMPILER "${ZIG_EXECUTABLE}")
set(CMAKE_CXX_COMPILER_ARG1 c++)
set(CMAKE_C_COMPILER_TARGET "${CMAKE_SYSTEM_PROCESSOR}-windows-gnu")
set(CMAKE_CXX_COMPILER_TARGET "${CMAKE_SYSTEM_PROCESSOR}-windows-gnu")

# Package configuration often compiles probe executables. Cross-compiled
# binaries cannot run on the Linux build host, so keep probes link-only.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
