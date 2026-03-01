#!/usr/bin/env bash
# Build the Voltorb Flip solver as a WebAssembly module using Emscripten.
# Prerequisites: Emscripten SDK must be installed and activated (source emsdk_env.sh).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_wasm"
OUTPUT_JS="$SCRIPT_DIR/docs/js/solver-wasm.js"
OUTPUT_WASM="$SCRIPT_DIR/docs/js/solver-wasm.wasm"

echo "==> Configuring WASM build..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

emcmake cmake "$SCRIPT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DVOLTORB_ENABLE_THREADS=OFF \
    -DVOLTORB_BUILD_WASM=ON \
    -DVOLTORB_BUILD_TESTS=OFF

echo "==> Building..."
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
emmake make voltorb_wasm -j"$NPROC"

echo "==> Copying output files..."
cp "$BUILD_DIR/voltorb_wasm.js" "$OUTPUT_JS"
cp "$BUILD_DIR/voltorb_wasm.wasm" "$OUTPUT_WASM"

echo "==> Done!"
echo "   JS:   $OUTPUT_JS"
echo "   WASM: $OUTPUT_WASM"
