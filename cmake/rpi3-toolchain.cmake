# CMake toolchain file for cross-compiling to Raspberry Pi 3B+ (aarch64)
# Uses the crosstool-ng toolchain at ~/x-tools/aarch64-rpi3-linux-gnu

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross-compiler paths
set(TOOLCHAIN_DIR "$ENV{HOME}/x-tools/aarch64-rpi3-linux-gnu")
set(CROSS_PREFIX "${TOOLCHAIN_DIR}/bin/aarch64-rpi3-linux-gnu-")

set(CMAKE_C_COMPILER   "${CROSS_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_PREFIX}g++")
set(CMAKE_LINKER        "${CROSS_PREFIX}ld")
set(CMAKE_AR            "${CROSS_PREFIX}ar")
set(CMAKE_RANLIB        "${CROSS_PREFIX}ranlib")
set(CMAKE_STRIP         "${CROSS_PREFIX}strip")

# Sysroot
set(CMAKE_SYSROOT "${TOOLCHAIN_DIR}/aarch64-rpi3-linux-gnu/sysroot")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")

# Search for programs only on the host
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Search for libraries and includes only in the sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
