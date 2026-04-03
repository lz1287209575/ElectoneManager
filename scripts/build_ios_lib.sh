#!/usr/bin/env bash
# build_ios_lib.sh — Build the C++ static library for iOS consumption by the Swift app.
# Usage:
#   bash scripts/build_ios_lib.sh           → arm64 device (default)
#   bash scripts/build_ios_lib.sh simulator → x86_64+arm64 simulator
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR/.."

TARGET="${1:-device}"

if [[ "$TARGET" == "simulator" ]]; then
    BUILD_DIR="$ROOT/build-ios-sim"
    SDK="iphonesimulator"
    ARCH="arm64;x86_64"
    SYSROOT="$(xcrun --sdk iphonesimulator --show-sdk-path)"
    echo "Building for: iOS Simulator ($ARCH)"
else
    BUILD_DIR="$ROOT/build-ios"
    SDK="iphoneos"
    ARCH="arm64"
    SYSROOT="$(xcrun --sdk iphoneos --show-sdk-path)"
    echo "Building for: iOS Device ($ARCH)"
fi

cmake -B "$BUILD_DIR" \
    -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DCMAKE_OSX_SYSROOT="$SYSROOT" \
    -DEM_BUFFER_SIZE=256 \
    -DEM_SAMPLE_RATE=48000 \
    -DEM_ENABLE_UI=OFF \
    "$ROOT"

echo ""
echo "Building em_engine static library..."
cmake --build "$BUILD_DIR" --config Release --target em_engine -- -sdk "$SDK"

echo ""
echo "Done. Library location:"
find "$BUILD_DIR" -name "libem_engine.a" 2>/dev/null | head -5

echo ""
echo "To generate Xcode project (requires xcodegen):"
echo "  cd $ROOT/ios && xcodegen generate"
echo ""
echo "Or open Xcode manually and set:"
echo "  Bridging Header: ../src/ios/ElectoneManager-Bridging-Header.h"
echo "  Library Search:  $BUILD_DIR/Release-$SDK"
echo "  Other Linker:    -lem_engine -lc++"
