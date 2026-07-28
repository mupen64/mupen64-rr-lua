set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
# vcpkg's platform expressions recognize MinGW as Windows. The chainload
# toolchain below changes CMake itself to Windows for Zig.
set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/toolchains/zig-windows-gnu.cmake")
set(VCPKG_C_FLAGS_DEBUG "-fno-sanitize=all")
set(VCPKG_CXX_FLAGS_DEBUG "-fno-sanitize=all")

set(VCPKG_ENV_PASSTHROUGH PATH)
set(VCPKG_POLICY_DLLS_WITHOUT_LIBS enabled)
