#!/bin/bash
# build_macos_lib.sh — Build libem_engine.dylib for Flutter macOS
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$ROOT/build-macos-flutter"

echo "Building for: macOS (Flutter FFI)"

cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DEM_BUFFER_SIZE=256 \
    -DEM_SAMPLE_RATE=48000 \
    -DEM_ENABLE_UI=OFF \
    -DEM_FLUTTER=ON \
    "$ROOT"

cmake --build "$BUILD_DIR" --target em_engine_shared -j$(sysctl -n hw.ncpu)

DYLIB="$BUILD_DIR/libem_engine.dylib"
DEST="$ROOT/flutter_app/macos/Frameworks/libem_engine.dylib"
mkdir -p "$ROOT/flutter_app/macos/Frameworks"

if [ -f "$DYLIB" ]; then
    cp "$DYLIB" "$DEST"
    echo "Done. dylib → $DEST"
else
    echo "ERROR: $DYLIB not found"
    exit 1
fi
