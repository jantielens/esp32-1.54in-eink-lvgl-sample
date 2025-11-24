#!/bin/bash

# ESP32 Upload Script
# Uploads the compiled firmware to the ESP32 device

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Configuration
SKETCH_PATH="$SCRIPT_DIR/src/app/app.ino"
BUILD_PATH="$SCRIPT_DIR/build"
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
ARDUINO_CLI="$SCRIPT_DIR/bin/arduino-cli"

# Auto-detect port if not specified
if [ -z "$1" ]; then
    if [ -e "/dev/ttyUSB0" ]; then
        PORT="/dev/ttyUSB0"
        echo "Auto-detected port: $PORT"
    elif [ -e "/dev/ttyACM0" ]; then
        PORT="/dev/ttyACM0"
        echo "Auto-detected port: $PORT"
    else
        echo "Error: No serial device found at /dev/ttyUSB0 or /dev/ttyACM0"
        echo "Please specify port manually: $0 <port>"
        exit 1
    fi
else
    PORT="$1"
fi

echo "=== Uploading ESP32 Firmware ==="

# Check if arduino-cli exists in local bin directory
if [ ! -f "$ARDUINO_CLI" ]; then
    echo "Error: arduino-cli is not installed at $ARDUINO_CLI"
    echo "Please run setup.sh first"
    exit 1
fi

# Check if build directory exists
if [ ! -d "$BUILD_PATH" ]; then
    echo "Error: Build directory not found"
    echo "Please run build.sh first"
    exit 1
fi

# Detect connected boards
echo "Detecting connected boards..."
"$ARDUINO_CLI" board list

# Upload firmware
echo ""
echo "Uploading to port: $PORT"
echo "Board: $FQBN"
echo ""

"$ARDUINO_CLI" upload \
    --fqbn "$FQBN" \
    --port "$PORT" \
    --input-dir "$BUILD_PATH" \
    "$SKETCH_PATH"

echo ""
echo "=== Upload Complete ==="
echo "Firmware has been uploaded successfully!"
echo ""
echo "Usage: $0 [port]"
echo "Example: $0 /dev/ttyUSB0"
