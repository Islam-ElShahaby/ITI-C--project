#!/bin/bash
# Cross-compile the TelemetryService for Raspberry Pi 3B+ (aarch64)
set -e

cd "$(dirname "$0")/.."

BUILD_DIR="build-rpi"
TOOLCHAIN="cmake/rpi3-toolchain.cmake"

# Add cross-compiler to PATH
export PATH="${HOME}/x-tools/aarch64-rpi3-linux-gnu/bin:${PATH}"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with toolchain
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../${TOOLCHAIN}" \
    -DCMAKE_BUILD_TYPE=Release

# Build only the TelemetryService target
make TelemetryService -j$(nproc)
