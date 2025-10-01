#!/bin/bash

# Build script for graphs lambda
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
LAMBDA_DIR="$SCRIPT_DIR/graphs"
OUTPUT_DIR="$SCRIPT_DIR/tmp"

echo "Building graphs lambda..."

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Create a temporary build directory
BUILD_DIR=$(mktemp -d)
echo "Using build directory: $BUILD_DIR"

# Copy lambda code
cp -r "$LAMBDA_DIR"/* "$BUILD_DIR/"

# If requirements.txt exists, install dependencies
if [ -f "$BUILD_DIR/requirements.txt" ]; then
    echo "Installing dependencies..."
    pip install -r "$BUILD_DIR/requirements.txt" -t "$BUILD_DIR" --no-deps
fi

# Create the zip file
cd "$BUILD_DIR"
zip -r "$OUTPUT_DIR/graphs_lambda.zip" .

# Clean up
rm -rf "$BUILD_DIR"

echo "Lambda package created: $OUTPUT_DIR/graphs_lambda.zip"