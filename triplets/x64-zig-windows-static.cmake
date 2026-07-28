set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
# vcpkg's platform expressions recognize MinGW as Windows. The chainload
# toolchain below changes CMake itself to Windows for Zig.
set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/toolchains/zig-windows-gnu.cmake")

set(VCPKG_ENV_PASSTHROUGH PATH)
