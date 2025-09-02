#!/bin/bash

# Build script for TestMetal with FBX support
set -e

# Configuration
CC=clang
CFLAGS="-std=c99 -O2 -Wall -Wextra -fobjc-arc"
OBJC_FLAGS="-fobjc-arc"
LDFLAGS="-framework Metal -framework MetalKit -framework Cocoa -framework Foundation"

# Directories
SRC_DIR="."
BUILD_DIR="build_fbx"
APP_DIR="$BUILD_DIR/TestMetal.app/Contents/MacOS"
RESOURCES_DIR="$BUILD_DIR/TestMetal.app/Contents/Resources"

# Create build directory
mkdir -p "$BUILD_DIR"
mkdir -p "$APP_DIR"
mkdir -p "$RESOURCES_DIR"

echo "Building TestMetal with FBX support..."

# Compile C files
echo "Compiling C files..."
$CC $CFLAGS -c "$SRC_DIR/engine_main.c" -o "$BUILD_DIR/engine_main.o"
$CC $CFLAGS -c "$SRC_DIR/engine_asset_fbx.c" -o "$BUILD_DIR/engine_asset_fbx.o"
$CC $CFLAGS -c "$SRC_DIR/engine_model.c" -o "$BUILD_DIR/engine_model.o"
$CC $CFLAGS -c "$SRC_DIR/engine_math.c" -o "$BUILD_DIR/engine_math.o"

# Compile Objective-C files
echo "Compiling Objective-C files..."
$CC $OBJC_FLAGS -c "$SRC_DIR/engine_metal.m" -o "$BUILD_DIR/engine_metal.o"
$CC $OBJC_FLAGS -c "$SRC_DIR/AppDelegate.m" -o "$BUILD_DIR/AppDelegate.o"
$CC $OBJC_FLAGS -c "$SRC_DIR/GameViewController.m" -o "$BUILD_DIR/GameViewController.o"
$CC $OBJC_FLAGS -c "$SRC_DIR/main.m" -o "$BUILD_DIR/main.o"

# Compile Metal shaders
echo "Compiling Metal shaders..."
xcrun metal -c "$SRC_DIR/Shaders.metal" -o "$BUILD_DIR/Shaders.air"
xcrun metallib "$BUILD_DIR/Shaders.air" -o "$BUILD_DIR/Shaders.metallib"

# Link everything together
echo "Linking..."
$CC $LDFLAGS \
    "$BUILD_DIR/engine_main.o" \
    "$BUILD_DIR/engine_asset_fbx.o" \
    "$BUILD_DIR/engine_model.o" \
    "$BUILD_DIR/engine_math.o" \
    "$BUILD_DIR/engine_metal.o" \
    "$BUILD_DIR/AppDelegate.o" \
    "$BUILD_DIR/GameViewController.o" \
    "$BUILD_DIR/main.o" \
    -o "$APP_DIR/TestMetal"

# Copy assets
echo "Copying assets..."
cp -r "$SRC_DIR/assets" "$RESOURCES_DIR/"
cp "$BUILD_DIR/Shaders.metallib" "$RESOURCES_DIR/"

echo "Build complete! App is at: $BUILD_DIR/TestMetal.app"
echo "You can run it with: open $BUILD_DIR/TestMetal.app"
