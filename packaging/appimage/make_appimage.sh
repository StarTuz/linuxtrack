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
# Try to find qmake6 or qmake
export QMAKE=$(which qmake6 || which qmake)
echo "Using qmake: $QMAKE"

# Run linuxdeploy
# --appdir: target AppDir
# --plugin qt: use Qt plugin to bundle Qt libs
# --output appimage: create the actual file
export LD_LIBRARY_PATH="$(pwd)/$APP_DIR/usr/lib:$(pwd)/$APP_DIR/usr/lib/linuxtrack:$LD_LIBRARY_PATH"
"$LINUXDEPLOY" --appdir "$APP_DIR" \
    --desktop-file "$APP_DIR/usr/share/applications/$APP_NAME.desktop" \
    --icon-file "$APP_DIR/usr/share/icons/hicolor/scalable/apps/$APP_NAME.svg" \
    --plugin qt \
    --output appimage

echo "--> Done! AppImage created."
ls -lh *.AppImage
