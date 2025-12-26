# Linuxtrack Modernization - Handoff Document

**Last Updated:** 2025-12-26
**Author:** Antigravity AI Assistant
**Project:** Linuxtrack Head Tracking Software
**Repository:** /home/startux/Code/linuxtrackfixed/linuxtrack

---

## 0. Project Landscape

> **IMPORTANT:** This section provides context about related projects. Read this first.

There are THREE related head tracking projects in this workspace:

### linuxtrackfixed (THIS PROJECT)
**Path:** `/home/startux/Code/linuxtrackfixed/linuxtrack`  
**Philosophy:** Slow, methodical modernization of the original linuxtrack  
**Goal:** Get linuxtrack compiling and working on modern Linux without breaking existing functionality  
**Approach:** Minimal, careful changes. Fix one thing at a time. Document everything.  
**Status:** Active - ✅ Qt6 Migration Complete, TrackIR Game Support Fixed. Follow MODERNIZATION_ROADMAP.md

### tuxtracksold
**Path:** `/home/startux/Code/tuxtracksold/`  
**Philosophy:** First aggressive modernization attempt  
**What Worked:**
- Qt6 compatibility
- Tracking window with camera view and 3D view (modernized for Wayland, backwards compatible with X11)
- X-Plane integration improvements  

**What Failed:** Wine bridge never worked  
**Status:** Archived/Reference only - contains useful code snippets but is not the active project  
**Access Rule:** Only access when explicitly instructed

### tuxtracks (NEW)
**Path:** `/home/startux/Code/tuxtracks/` (when created)  
**Philosophy:** Completely new, modern system built from the ground up  
**Goal:** Modern Linux TrackIR head tracking for today and the future  
**Approach:** Greenfield development, no legacy constraints  
**Status:** New project - will have its own documentation and workflow rules

### Shared Documentation
**Path:** `/home/startux/Code/HEAD_TRACKING_PROTOCOL.md`  
**Purpose:** Protocol specifications that ALL projects must follow for game compatibility

---

## 1. Project Overview

### What is Linuxtrack?

Linuxtrack is a comprehensive head-tracking solution for Linux and Mac that enables users to control applications (primarily games and flight simulators) using head movement. It supports multiple input devices and provides compatibility with Windows games through a Wine bridge.

### Supported Hardware

| Device | Linux Support | Notes |
|--------|---------------|-------|
| TrackIR (1-5) | ✅ Full | USB infrared tracking devices |
| Webcam | ✅ Full | V4L2 webcams with blob tracking |
| Facetracker | ✅ Full | OpenCV-based face tracking |
| Wiimote | ⚠️ Optional | Requires libcwiid |
| PS3 Eye | ✅ Full | Specialized USB camera |

### Key Components

| Component | Type | Description |
|-----------|------|-------------|
| `ltr_gui` | Qt Application | Main configuration GUI |
| `ltr_server1` | Daemon | Background tracking service |
| `liblinuxtrack.so` | Library | Client library for applications |
| `libltr.so` | Library | Core tracking library |
| `libtir.so` | Plugin | TrackIR device driver |
| `libwc.so` | Plugin | Webcam driver |
| `libft.so` | Plugin | Face tracker (OpenCV) |
| `xlinuxtrack9.so` | Plugin | X-Plane 9+ integration |
| `NPClient.dll.so` | Wine DLL | TrackIR emulation for Windows games |
| `FreeTrackClient.dll.so` | Wine DLL | FreeTrack emulation for Windows games |
| `mickey` | Qt Application | Mouse control via head tracking |
| `osc_server` | Server | OSC protocol output for external apps |

---

## 2. Current Session Summary

### Objective

Fix Linuxtrack to compile on modern Linux systems without significant architectural changes.

### Status: ✅ COMPLETE

The project now compiles successfully on modern Linux with:
- GCC 14.x
- Wine 9.x  
- Qt 5.x/6.x
- OpenCV 4.x
- Autotools (automake 1.18, autoconf 2.72)

---

## 3. Changes Made

### 3.1 Missing pthread.h Includes

**Problem:** Modern GCC has stricter header isolation. `pthread.h` is no longer implicitly included through other headers.

**Files Modified:**

1. **`src/ltr_srv_master.cpp`** (Line 3)
   ```cpp
   #include <pthread.h>
   ```

2. **`src/pref.cpp`** (Line 5)
   ```cpp
   #include <pthread.h>
   ```

**Error Fixed:**
```
error: 'PTHREAD_MUTEX_INITIALIZER' was not declared in this scope
```

---

### 3.2 Wine DLL_WINE_PREATTACH Removal

**Problem:** `DLL_WINE_PREATTACH` was an internal Wine extension removed in Wine 6.0 (February 2021).

**Files Modified:**

1. **`src/wine_bridge/client/NPClient_main.c`** (Lines 55-58)
   ```c
   #ifdef DLL_WINE_PREATTACH
       case DLL_WINE_PREATTACH:
           return TRUE;
   #endif
   ```

2. **`src/wine_bridge/ft_client/FreeTrackClient_main.c`** (Lines 63-67)
   ```c
   #ifdef DLL_WINE_PREATTACH
   case DLL_WINE_PREATTACH:
       return TRUE;
   #endif
   ```

**Error Fixed:**
```
error: 'DLL_WINE_PREATTACH' undeclared (first use in this function)
```

**Note:** Using `#ifdef` maintains backward compatibility with older Wine versions.

---

### 3.3 Wine Debug Channel Name Length

**Problem:** Wine debug channel names have a 15-character limit (14 + NUL). "FreeTrackClient" is exactly 15 characters.

**File Modified:**

**`src/wine_bridge/ft_client/FreeTrackClient_main.c`** (Line 22)
```c
// Before:
WINE_DEFAULT_DEBUG_CHANNEL(FreeTrackClient);

// After:
WINE_DEFAULT_DEBUG_CHANNEL(FreeTrackCli);
```

**Error Fixed:**
```
error: static assertion failed: "sizeof(\"FreeTrackClient\") <= sizeof(__wine_dbch_FreeTrackClient.name)"
```

---

### 3.4 Format Specifier Fix (Partial)

**Problem:** `DWORD` type is `unsigned int` on 32-bit Wine and `unsigned long` on 64-bit Wine.

**Files Partially Fixed:**
- `NPClient_main.c`: Changed `%d` to `%ld` for DWORD
- `FreeTrackClient_main.c`: Changed to `%lu` with cast

**Remaining Warnings (Non-blocking):**
- `src/wine_bridge/views/rest.c:219` - `%d` for `GetLastError()`

---

### 3.5 Qt6 Migration & 3D View Modernization
**Status:** ✅ COMPLETE

**Problem:** `QGLWidget` and fixed-function OpenGL (`glBegin`, `glCallList`) are deprecated in Qt6 and do not work on Wayland/EGL.

**Fixes:**
1.  **Dual Support**: Updated headers and `.pro` files to support both Qt5 and Qt6.
2.  **Modern OpenGL**: Completely refactored `GLWidget` for Qt6 to use `QOpenGLWidget`, `QOpenGLVertexArrayObject`, `QOpenGLBuffer`, and `QOpenGLShaderProgram`.
3.  **Shaders**: Implemented GLSL ES 3.0 shaders for vertex and fragment processing.
4.  **Transformation Logic**: Migrated from OpenGL matrix stack to `QMatrix4x4` for MVP and normal matrices.
5.  **Geometry**: Converted on-the-fly OBJ parsing into static VBO/VAO buffers for performance and Wayland compatibility.

### 3.6 Wine Bridge & Path Fixes
**Status:** ✅ COMPLETE

**Problem:** Typos in `Makefile.am` and missing RPATH logic in the Wine bridge bridge caused games (like Elite Dangerous) to fail to find tracking libraries after the rebuild.

**Fixes:**
1.  **Global Fix**: Fixed typos in `src/Makefile.am` (`LINUXLDLFAGS` -> `LINUXLDFLAGS`).
2.  **Bridge Fix**: Added `-Wl,-rpath` flags to `src/wine_bridge/client/Makefile.am` and `src/wine_bridge/ft_client/Makefile.am`.
3.  **Relocatable Binaries**: Used `$ORIGIN` to ensure binaries can find libraries in both build and install (`/opt/linuxtrack/lib/linuxtrack`) locations.

### 3.7 Segmentation Fault Fixes
**Status:** ✅ COMPLETE

**Problem:** `ltr_gui` crashed on exit during cleanup.

**Fixes:**
1.  **ProfileSelector**: Added null check for `ProfileSetup` pointer during close event.
2.  **GLWidget Destructor**: Implemented safe, unique-pointer deletion for textures to avoid double-free errors.
3.  **Context Management**: Ensured `makeCurrent()` / `doneCurrent()` calls wrap GL resource deletion.

---

### 3.8 Preference System Modernization (Bison Removal)
**Status:** ✅ COMPLETE

**Problem:** The legacy Bison/Flex parser for `.conf` files was brittle, hard to maintain, and prone to build issues with modern tools.

**Fixes:**
1.  **Modern Backend**: Migrated to the `mINI` header-only library for robust INI file parsing.
2.  **Bridge Layer**: Implemented a bridge in `pref.cpp` that maintains compatibility with the existing C API (`ltr_int_get_key`, etc.) and the legacy C++ `prefs` class used by the GUI.
3.  **Cleanup**: Removed `pref_bison.ypp` and `pref_flex.lpp` from the build system, simplifying project dependencies.

### 3.9 GUI Polish & Qt6 Fixes
**Status:** ✅ COMPLETE

**Problem:** Deprecated Qt5 signals or broken auto-connections in Qt6.

**Fixes:**
1.  **Model Setup**: Fixed the "Model Setup" images not updating by manually connecting `QComboBox::textActivated` to a new slot, bypassing the removed `activated(QString)` signal.

### 3.10 Wine Support Modernization (Proton Compatibility)
**Status:** ✅ COMPLETE

**Problem:** The legacy Wine bridge installation relied on a fragile NSIS installer, downloading 20-year-old Microsoft runtimes (VC6), and using "Fake Wine" environments to extract firmware, which is incompatible with modern Proton/Steam Deck environments.

**Fixes:**
1.  **Surgical Injection**: Implemented a native "Surgical Injection" routine in `PluginInstall::installLinuxtrackWine`. It now directly copies compiled bridge files (`NPClient.dll`, `FreeTrackClient.dll`, `Controller.exe`) to the prefix and updates the registry via native `wine reg` calls.
2.  **Controller.exe for Hotkeys**: Now includes `Controller.exe` in surgical injection for customizable Pause/Recenter hotkeys. Uses DirectInput for global keyboard capture that works even when games have focus.
3.  **Dual Program Files Installation**: Installs to both `C:\Program Files\Linuxtrack\` and `C:\Program Files (x86)\Linuxtrack\` for 32-bit and 64-bit game compatibility.
4.  **Proton Tuning**: `WineLauncher` now automatically sets `PROTON_NO_FSYNC=1` and `PROTON_NO_ESYNC=1` to prevent race conditions in tracking data.
5.  **Automatic Prefix Discovery**:
    - ✅ **Qt6 entryList Fix**: Fixed Steam game discovery by switching from Qt's glob patterns (broken in Qt6 with C locale) to `entryInfoList` with manual filtering.
    - ✅ **Steam/Proton**: Scans all Steam library folders including custom mount points.
    - ✅ **Lutris**: Parses game YAML configs for Wine prefixes.
    - ✅ **Bottles**: Detects Flatpak and native Bottles installations.
    - ✅ **X-Plane 12**: Auto-detection for native X-Plane installation paths.
6.  **Native Harvesting**: Refactored firmware extraction to prioritize native tools (`7z`). Can unpack TrackIR installers without Wine.
7.  **Verification**: 
    - ✅ **DCS World**: Confirmed successful with Controller.exe hotkeys (Proton).
    - ✅ **Elite Dangerous**: Confirmed successful on **Proton 10**.
    - ✅ **X-Plane 12**: Native plugin confirmed working.
    - ✅ **Fresh Prefixes**: Confirmed tracking works on fresh 64-bit Wine prefixes (Registry fix).

### 3.11 Deprecated API Replacement & Code Safety
**Status:** ✅ COMPLETE

**Problem:** Several deprecated C APIs (`sprintf`, `strtok`) posed buffer overflow and thread-safety risks.

**Fixes:**
1.  **Wine Bridge (High Priority)**:
    - `wine_bridge/client/check_data.c`: `sprintf` → `snprintf` with tracked buffer size
    - `wine_bridge/client/rest.c`: All path construction now uses `snprintf`
    - `wine_bridge/ft_tester/main.cpp`: `sprintf` → `snprintf` for DLL path
2.  **Core Library**:
    - `webcam_driver.c`: `snprintf` for debug frame filenames
    - `image_process.c`: `snprintf` for debug data filenames
    - `linuxtrack.c`: `strtok` → `strtok_r` for thread-safe `LINUXTRACK_LIBS` parsing

### 3.13 GLWidget Race Condition Fix
**Status:** ✅ COMPLETE

**Problem:** 3D View displayed black due to race condition between `ReaderThread` (loading model objects) and `initializeGL()` (building OpenGL buffers).

**Fix:**
1.  Added `objectsLoaded` and `glInitialized` flags to `GLWidget`.
2.  `initializeGL()` sets `glInitialized = true` and calls `makeObjects()` if `objectsLoaded` is already true.
3.  `objectsRead()` sets `objectsLoaded = true` and calls `makeObjects()` if `glInitialized` is already true.

### 3.14 Fresh Prefix & Proton Fix (Critical)
**Status:** ✅ COMPLETE

**Problem:** Fresh 64-bit Wine/Proton prefixes (like a new Elite Dangerous install) failed to track. Surgical injection appeared successful, but games couldn't find `NPClient.dll`.

**Root Cause:**
The `WineLauncher` helper class was forcing `WINEARCH=win32` environment variable for all internal commands. When running `reg.exe` to inject registry keys into a 64-bit prefix, this mismatch caused the registry write to fail or go to the wrong hive, leaving the game without the necessary `HKCU\Software\NaturalPoint` keys.

**Fix:**
- Removed `WINEARCH=win32` from `WineLauncher.cpp`. The launcher now respects the prefix's native architecture.
- Added `PROTON_NO_FSYNC=1` and `PROTON_NO_ESYNC=1` to the `WineLauncher` environment to protect internal tools, but **removed** the requirement for users to set this manually.
- **Verification:** Validated that tracking works out-of-the-box on fresh prefixes without manual Steam Launch Options.

### 3.15 Project LAL (Licensed Asset Loader)
**Status:** ✅ COMPLETE (Prototype)

**Problem:** The legacy method of extracting proprietary firmware (TIRViews.dll) relied on a fragile Wine-based "fake environment" that was hard to maintain and security-prone.

**Fix:**
1.  **LALManager**: Implemented a new C++ backend that natively extracts assets using `7z` or `tar`, with no Wine dependency.
2.  **Manifest-Driven**: Assets are defined in `lal_manifest.json`, making updates easy without recompiling.
3.  **LALDialog**: Added a native Qt GUI dialog ("Manage Assets (LAL)...") to the Misc tab, allowing users to browse and install firmware archives easily.
4.  **Verification**: Verified UI functionality and native extraction logic via unit tests.

### 3.16 AppImage Packaging
**Status:** ✅ COMPLETE

**Problem:** Running `ltr_gui` from the build directory caused permissions/loading issues, and the traditional `sudo make install` approach is difficult to distribute to end users.

**Fix:**
1.  **Workflow**: Created `.github/workflows/build.yml` to automatically build an AppImage on every push.
2.  **Script**: Added `packaging/appimage/make_appimage.sh` which uses `linuxdeploy` and `linuxdeploy-plugin-qt`.
3.  **Result**: Users can now download a single, portable `Linuxtrack-x86_64.AppImage` executable that works on most modern Linux distributions (Ubuntu 22.04+, Fedora, Arch, etc.).

---

## 4. Build Configuration

### Configure Output Summary

```
Webcam support........... yes
Wiimote support.......... no (libcwiid not found)
TrackIR support.......... yes
Facetracker support...... yes (OpenCV4)
XPlane plugin............ yes
Mickey................... yes
Wine plugin.............. yes (32-bit + 64-bit)
OSC support.............. yes
```

### Build Commands (Modern)

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
# Optional: sudo cmake --install .
```

### Required Dependencies (Arch Linux)

```bash
# Core build tools
base-devel cmake ninja pkg-config

# Libraries
mxml libusb zlib

# Qt (for GUI)
qt6-base qt6-tools qt6-5compat qt6-help

# Video/Webcam
v4l-utils

# OpenCV (for facetracker)
opencv

# Wine (for Windows game bridge)
wine wine-mono wine-gecko

# Optional dependencies
liblo                    # OSC support
xplane-sdk               # X-Plane plugin
libcwiid                 # Wiimote support
```

---

## 5. Build Artifacts

### Libraries (in `src/.libs/`)

| File | Description |
|------|-------------|
| `liblinuxtrack.so` | Main client library |
| `liblinuxtrack32.so` | 32-bit client library |
| `libltr.so` | Core tracking library |
| `libtir.so` | TrackIR driver |
| `libltusb1.so` | libusb-1.0 interface |
| `libwc.so` | Webcam driver |
| `libft.so` | Face tracker driver |
| `libjoy.so` | Joystick driver |
| `libfakeusb.so` | USB emulation |
| `libmacwii.so` | Mac Wiimote driver |
| `libp3e.so` | PS3 Eye driver |
| `libp3eft.so` | PS3 Eye + facetracker |
| `xlinuxtrack9.so` | X-Plane 9+ plugin |
| `xlinuxtrack9_32.so` | X-Plane 32-bit plugin |

### Executables (in `src/`)

| File | Description |
|------|-------------|
| `ltr_server1` | Main tracking server daemon |
| `ltr_gui` | Qt configuration GUI |
| `ltr_pipe` | Pipe-based tracking interface |
| `ltr_extractor` | Game data extractor |
| `ltr_recenter` | Recenter command utility |
| `osc_server` | OSC protocol server |
| `mickey` | Mouse control application |
| `ltr_udp` | X4: Foundations (OpenTrack UDP) bridge |

### Wine Bridge (in `src/wine_bridge/`)

| File | Description |
|------|-------------|
| `client/NPClient.dll.so` | 32-bit TrackIR emulation DLL |
| `client/NPClient64.dll.so` | 64-bit TrackIR emulation DLL |
| `ft_client/FreeTrackClient.dll.so` | FreeTrack emulation DLL |
| `linuxtrack-wine.exe` | NSIS installer for Wine prefix |
| `controller/Controller.exe` | DirectInput controller |
| `tester/Tester.exe` | 32-bit testing utility |
| `tester/Tester64.exe` | 64-bit testing utility |
| `views/TrackIR.exe` | TrackIR status viewer |

---

## 6. Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        Applications                              │
├─────────────┬─────────────┬─────────────┬──────────────────────┤
│  X-Plane    │   Native    │   OSC       │   Windows Games      │
│  Plugin     │   Linux     │   Clients   │   (via Wine)         │
│xlinuxtrack9 │   Apps      │             │                      │
├─────────────┴─────────────┴─────────────┴──────────────────────┤
│                     liblinuxtrack.so                            │
│                  (Client Library API)                           │
├─────────────────────────────────────────────────────────────────┤
│                      ltr_server1                                │
│                 (Background Daemon)                             │
├─────────────────────────────────────────────────────────────────┤
│                       libltr.so                                 │
│                 (Core Tracking Engine)                          │
├─────────────┬─────────────┬─────────────┬──────────────────────┤
│   libtir    │   libwc     │   libft     │   libp3e            │
│  (TrackIR)  │  (Webcam)   │ (Facetrack) │  (PS3 Eye)          │
├─────────────┴─────────────┴─────────────┴──────────────────────┤
│                     Hardware Devices                            │
│          TrackIR USB | Webcam V4L2 | PS3 Eye USB               │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7. Known Issues & Limitations

### Current Issues

1. **Relocatable Installation** ✅ FIXED
   - CMake build now uses `$ORIGIN`-based RPATHs.
   - Binaries and libraries are relocatable and find each other automatically.
   - No longer dependent on fixed absolute paths in the binaries.

2. **Wiimote Support Disabled**
   - Requires `libcwiid` package
   - Install: `yay -S libcwiid` (AUR)

3. **Format Specifier Warnings**
   - `rest.c:219` has `%d` for `DWORD`
   - Non-blocking, cosmetic only

4. **IDE Lint Errors**
   - Wine bridge files show false-positive errors in IDEs
   - Cause: IDE doesn't understand `winegcc` include paths
   - These files compile correctly with the Wine toolchain

### Historical Note

This codebase was last actively maintained around 2015-2018. The original author is "uglyDwarf" (GitHub). The fixes in this session address API changes in:
- GCC (stricter headers)
- Wine 6.0+ (removed internal APIs)
- Modern autotools

---

## 8. Next Steps (Future Work)

The user mentioned "there will be more later." Potential future work could include:

1. **Testing & Validation**
   - Test with actual TrackIR hardware
   - Test Wine bridge with games
   - Verify X-Plane plugin functionality

2. **Code Modernization**
   - Update remaining format specifiers
   - Consider C++11/14 modernization
   - Add unit tests

3. **Build System**
   - Consider CMake migration
   - CI/CD pipeline setup

4. **Documentation**
   - Update wiki
   - Document Wine prefix setup

---

## 9. Files Created/Modified

### Created

| File | Purpose |
|------|---------|
| `HANDOFF.md` | This document |
| `COMPILATION_FIXES.md` | Technical fix details |
| `INSTALLATION_PATH_ANALYSIS.md` | **CRITICAL:** RPATH and installation path analysis |
| `MODERNIZATION_ROADMAP.md` | Future improvements roadmap (Qt6, CI/CD, etc.) |

> ⚠️ **CRITICAL:** If linuxtrack compiles but doesn't track in games, read `INSTALLATION_PATH_ANALYSIS.md` first! The build system has complex path dependencies involving RPATH, compile-time macros, and runtime configuration.

### Modified

| File | Change |
|------|--------|
| `src/qt_gui/wine_launcher.cpp` | Removed `WINEARCH=win32` to fix 64-bit registry injection |
| `src/CMakeLists.txt` | Complete CMake build system implementation |
| `src/wine_bridge/client/rest.c` | Fixed format specifier for `GetLastError()` |
| `src/ltr_srv_master.cpp` | Added `#include <pthread.h>` |
| `src/pref.cpp` | Added `#include <pthread.h>` |
| `src/wine_bridge/client/NPClient_main.c` | Wine 6.0+ compatibility |

---

## 10. Git Status

### Uncommitted Changes

```
Modified:
  src/qt_gui/wine_launcher.cpp
  src/qt_gui/plugin_install.cpp
  HANDOFF.md (this file)
  MODERNIZATION_ROADMAP.md

Untracked:
  INSTALLATION_PATH_ANALYSIS.md
  COMPILATION_FIXES.md
```

### Recommended Commit Message

```
fix: Resolve 64-bit Wine prefix tracking issue and modernize build

- **Fix:** Removed forced `WINEARCH=win32` in WineLauncher. This fixes `reg.exe` usage on 64-bit prefixes, ensuring Linuxtrack registry keys are correctly injected.
- **Fix:** Added `PROTON_NO_FSYNC/ESYNC` tunable Protection to internal tools to prevent race conditions.
- **Modernization:** Verified Qt6 migration and "Surgical Injection" bridge.
- **Docs:** Updated HANDOFF.md and MODERNIZATION_ROADMAP.md with verified status.

This commit makes Linuxtrack fully "Release Ready" for modern Linux distributions and Proton games (Elite Dangerous, DCS, etc.).
```

---

## 11. Contact & Resources

- **Original Project:** https://github.com/uglyDwarf/linuxtrack
- **Wiki:** https://github.com/uglyDwarf/linuxtrack/wiki
- **Issue Tracker:** https://github.com/uglyDwarf/linuxtrack/issues

---

*End of Handoff Document*
