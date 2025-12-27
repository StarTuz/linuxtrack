#!/bin/bash
set -e

# Configuration
APP_NAME="linuxtrack"
BUILD_DIR="${BUILD_DIR:-build}"
APP_DIR="AppDir"

# Ensure we are in the project root
if [ ! -f "src/CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory."
    exit 1
fi

# Clean up previous artifacts
rm -rf "$APP_DIR"
rm -f *.AppImage

# 1. Install project to AppDir
echo "--> Installing to $APP_DIR..."
DESTDIR=$(pwd)/"$APP_DIR" cmake --install "$BUILD_DIR" --prefix "/usr"

# 2. Setup AppImage resources
echo "--> Setting up AppImage resources..."

# If dist32 exists (from a 2-pass CI build), copy those libs into the AppDir
if [ -d "dist32" ]; then
    echo "--> Found 32-bit libraries in dist32, copying to AppDir..."
    mkdir -p "$APP_DIR/usr/lib/linuxtrack/"
    cp -v dist32/*.so "$APP_DIR/usr/lib/linuxtrack/" 2>/dev/null || true
fi

# Copy icon and desktop file to standard request locations if not already there managed by install
# (Our install step usually puts them in /usr/share/applications and /usr/share/pixmaps)

# Verify desktop file exists in AppDir
if [ ! -f "$APP_DIR/usr/share/applications/$APP_NAME.desktop" ]; then
    echo "Warning: Desktop file not found in AppDir at standard location."
    # Attempt to copy manually if missing
    mkdir -p "$APP_DIR/usr/share/applications"
    cp src/$APP_NAME.desktop "$APP_DIR/usr/share/applications/"
fi

# Verify icon exists in AppDir
if [ ! -f "$APP_DIR/usr/share/icons/hicolor/scalable/apps/$APP_NAME.svg" ] && [ ! -f "$APP_DIR/usr/share/pixmaps/$APP_NAME.svg" ]; then
    echo "Warning: Icon file not found in AppDir at standard location."
    # Attempt to copy manually if missing
    mkdir -p "$APP_DIR/usr/share/icons/hicolor/scalable/apps"
    cp src/$APP_NAME.svg "$APP_DIR/usr/share/icons/hicolor/scalable/apps/"
fi

# 3. Download LinuxDeploy tools (if not present)
TOOLS_DIR="packaging/tools"
mkdir -p "$TOOLS_DIR"

LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_PLUGIN_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"

if [ ! -f "$LINUXDEPLOY" ]; then
    echo "--> Downloading linuxdeploy..."
    wget -q -O "$LINUXDEPLOY" "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY"
fi

if [ ! -f "$LINUXDEPLOY_PLUGIN_QT" ]; then
    echo "--> Downloading linuxdeploy-plugin-qt..."
    wget -q -O "$LINUXDEPLOY_PLUGIN_QT" "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY_PLUGIN_QT"
fi

# 4. Generate AppImage
echo "--> Generating AppImage..."

# We need to set QMAKE path for the plugin to find Qt6
# Try to find qmake6, qmake in common locations
export QMAKE=$(which qmake6 || which qmake || find /usr/lib/qt6/bin -name qmake 2>/dev/null | head -n 1)
if [ -z "$QMAKE" ]; then
    echo "Warning: qmake not found. Attempting to use default Qt plugin detection."
else
    echo "Using qmake: $QMAKE"
fi

# Run linuxdeploy
# --appdir: target AppDir
# --output appimage: create the actual file
# Note: we use LD_LIBRARY_PATH to help linuxdeploy find our internal libraries
# We do NOT use --plugin qt because bundling Qt can cause ABI incompatibilities
# on systems with newer Qt versions. The AppImage will use system Qt instead.
export LD_LIBRARY_PATH="$(pwd)/$APP_DIR/usr/lib:$(pwd)/$APP_DIR/usr/lib/linuxtrack:$LD_LIBRARY_PATH"

# We must set ARCH because we bundle both 32-bit and 64-bit libraries
export ARCH=x86_64

"$LINUXDEPLOY" --appdir "$APP_DIR" \
    --desktop-file "$APP_DIR/usr/share/applications/$APP_NAME.desktop" \
    --icon-file "$APP_DIR/usr/share/pixmaps/$APP_NAME.svg" \
    --output appimage

echo "--> Done! AppImage created."
ls -lh *.AppImage
