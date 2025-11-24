#!/bin/bash

# ESP32 Build Script
# Compiles the Arduino sketch to the /build directory

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Configuration
SKETCH_PATH="$SCRIPT_DIR/src/app/app.ino"
BUILD_PATH="$SCRIPT_DIR/build"
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"  # ESP32 Dev Module with minimal SPIFFS partition (larger app space)
ARDUINO_CLI="$SCRIPT_DIR/bin/arduino-cli"

echo "=== Building ESP32 Firmware ==="

# Check if arduino-cli exists in local bin directory
if [ ! -f "$ARDUINO_CLI" ]; then
    echo "Error: arduino-cli is not installed at $ARDUINO_CLI"
    echo "Please run setup.sh first"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p "$BUILD_PATH"

# Compile the sketch
echo "Compiling sketch: $SKETCH_PATH"
echo "Board: $FQBN"
echo "Output directory: $BUILD_PATH"
echo ""

"$ARDUINO_CLI" compile \
    --fqbn "$FQBN" \
    --output-dir "$BUILD_PATH" \
    "$SKETCH_PATH"

echo ""
echo "=== Build Complete ==="
echo "Firmware binary created in: $BUILD_PATH"
ls -lh "$BUILD_PATH"/*.bin 2>/dev/null || echo "Binary files generated"
