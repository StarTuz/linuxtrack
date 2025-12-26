# Linuxtrack Modernization Roadmap

**Date:** 2025-12-25  
**Status:** Planning Document  
**Purpose:** Track future modernization work without breaking existing functionality

---

## Overview

This document outlines incremental modernization steps for linuxtrack, prioritized by **safety** (least likely to break things) and **impact** (biggest improvements for users/developers).

---

## Tier 1: Safe & High Impact (Do First)

### 1.1 Use `$ORIGIN` in RPATH for Relocatable Binaries

**Status:** ✅ COMPLETE (2025-12-25)  
**Effort:** 15 minutes  
**Risk:** Very Low

**Problem:** Currently, RPATH is an absolute path baked into binaries. If you move the installation, it breaks.

**Solution:** Use `$ORIGIN` to make paths relative to the binary location.

**Files Changed:** `src/Makefile.am` lines 19 and 27

```makefile
# Line 19 - Main RPATH:
LINUXLDLFAGS = '-Wl,-rpath,$$ORIGIN/../lib/linuxtrack'

# Line 27 - Face tracker RPATH:
FT_LDFLAGS = '-Wl,-rpath,$$ORIGIN/../lib'
```

**Benefits:**
- Installation becomes fully relocatable
- Users can install anywhere without recompiling
- Package maintainers have more flexibility

---

### 1.2 Add Runtime Path Validation with Clear Error Messages

**Status:** ✅ COMPLETE (2025-12-25)  
**Effort:** 30 minutes  
**Risk:** Very Low

**Problem:** If libraries aren't found, tracking silently fails. Users see no error.

**Solution:** Added startup validation in `src/ltr_server1.c` with `validate_paths()` function:

```c
#include <unistd.h>

static void validate_library_paths(void) {
    const char *lib_path = LIB_PATH "libltr.so.0";
    if (access(lib_path, F_OK) != 0) {
        fprintf(stderr, "ERROR: Cannot find libltr.so.0\n");
        fprintf(stderr, "Expected location: %s\n", lib_path);
        fprintf(stderr, "This binary was compiled for prefix: %s\n", PREFIX);
        fprintf(stderr, "\nPossible solutions:\n");
        fprintf(stderr, "  1. Reinstall to the correct location\n");
        fprintf(stderr, "  2. Set LD_LIBRARY_PATH=%s\n", LIB_PATH);
        fprintf(stderr, "  3. Rebuild with correct --prefix\n");
        exit(1);
    }
}
```

**Benefits:**
- Users immediately know why tracking fails
- Reduces debugging time significantly

---

### 1.3 Fix Remaining Format Specifier Warnings

**Status:** ✅ COMPLETE (2025-12-25)  
**Effort:** 15 minutes  
**Risk:** Very Low (cosmetic only)

**Files Changed:**
- `src/wine_bridge/views/rest.c:219` - Fixed `%d` to `%lu` with cast for `GetLastError()`

**Pattern:**
```c
// Before:
TRACE("Value: %d\n", GetLastError());

// After:
TRACE("Value: %lu\n", (unsigned long)GetLastError());
```

---

## Tier 2: Moderate Effort, Good Payoff

### 2.1 Qt6 Support

**Status:** ✅ COMPLETE (2025-12-25)  
**Effort:** 2-3 hours  
**Risk:** Medium (but maintains Qt5 compatibility)

**Completed:**
- ✅ 2.1.1 Update configure.ac for Qt6 qmake detection (qmake6 prioritized)
- ✅ 2.1.2 `.pro` files updated with Qt6 conditionals
- ✅ 2.1.3 GLWidget Qt5/Qt6 dual-support (QOpenGLWidget base class, QOpenGLFunctions)
- ✅ 2.1.4 3D View Modernization: Ported legacy fixed-function OpenGL to Modern OpenGL (GLSL ES 3.0 shaders, VAO, VBO, QMatrix4x4) for Wayland/EGL compatibility
- ✅ 2.1.5 QRegExp → QRegularExpression migration (ltr_model, objreader, webcam_info, macwebcam_info)
- ✅ 2.1.6 QDesktopWidget → QScreen for mickey (mickey.h, mickey.cpp)
- ✅ 2.1.7 QX11Info → native X11 display for Qt6 (keyb_x11.h, keyb_x11.cpp)
- ✅ 2.1.8 Build verified with Qt6 (both ltr_gui and mickey binaries built successfully)
- ✅ 2.1.9 updateGL() → update() for Qt6 compatibility (window.cpp)
- ✅ 2.1.10 Fixed exit segmentation faults (null checks in ProfileSelector and GLWidget destructor)
- ✅ 2.1.11 Fixed TrackIR game support by adding RPATH/RUNPATH to Wine bridge binaries (NPClient64, FreeTrackClient)
- ✅ 2.1.12 Fixed Model Setup image updates (Qt6 textActivated signal bridge)
- ✅ 2.1.13 Replaced legacy Bison/Flex preference parser with modern mINI backend
- ✅ 2.1.14 Modernized Wine Support: Replaced fragile NSIS installer with native "Surgical Injection" (Direct file copy + Registry updates)
- ✅ 2.1.15 Proton-Aware Environment: Implemented automatic tuning (`PROTON_NO_FSYNC/ESYNC`) in `WineLauncher`
- ✅ 2.1.16 Native Firmware Harvesting: Implemented native `7z`-based extraction for firmware files, avoiding Wine for harvesting

**Problem:** Many modern distros are moving to Qt6 as default. Current code only supports Qt4/Qt5.

#### Changes Required

##### 2.1.1 Update configure.ac for Qt6 qmake detection

```m4
# Before:
AC_PATH_PROGS(QMAKE_PATH, [qmake qmake-qt4])

# After:
AC_PATH_PROGS(QMAKE_PATH, [qmake6 qmake-qt6 qmake qmake-qt5 qmake-qt4])
```

##### 2.1.2 Update .pro files for Qt6 conditionals

**`src/qt_gui/ltr_gui.pro`:**
```qmake
contains(QT_VERSION, ^5.*){
    QT += widgets
}

contains(QT_VERSION, ^6.*){
    QT += widgets openglwidgets
}
```

**`src/mickey/mickey.pro`:**
```qmake
contains(QT_VERSION, ^5.*){
    QT += widgets x11extras
    DEFINES += QT5_OVERRIDES
}

contains(QT_VERSION, ^6.*){
    QT += widgets
    DEFINES += QT6_OVERRIDES  
}
```

##### 2.1.3 Replace deprecated Qt classes

| Qt5 Class | Qt6 Replacement | Files Affected |
|-----------|-----------------|----------------|
| `QRegExp` | `QRegularExpression` | `xplugin.cpp`, `webcam_info.cpp`, `objreader.cpp`, `ltr_model.cpp`, `help_view.cpp`, `precomp_headers.h` |
| `QRegExpValidator` | `QRegularExpressionValidator` | `ltr_model.cpp`, `ltr_model.h` |
| `QDesktopWidget` | `QScreen` | `mickey.cpp`, `mickey.h`, `mouse_mac.cpp` |
| `x11extras` module | `QNativeInterface::QX11Application` | `mickey.pro` |

##### Example dual-support pattern:

```cpp
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #include <QRegularExpression>
    #include <QScreen>
    #include <QGuiApplication>
#else
    #include <QRegExp>
    #include <QDesktopWidget>
#endif

// QRegExp replacement
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QRegularExpression pathRexp(QString::fromUtf8("^(.*/)[^/]+$"));
    QRegularExpressionMatch match = pathRexp.match(path);
    if (match.hasMatch()) {
        QString captured = match.captured(1);
    }
#else
    QRegExp pathRexp(QString::fromUtf8("^(.*/)[^/]+$"));
    if (pathRexp.exactMatch(path)) {
        QString captured = pathRexp.cap(1);
    }
#endif

// QDesktopWidget replacement
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen ? screen->availableGeometry() : QRect();
#else
    QRect screenRect = QApplication::desktop()->availableGeometry(w);
#endif
```

---

### 2.2 Add GitHub Actions CI/CD

**Status:** ✅ COMPLETE (2025-12-25)  
**Effort:** 1 hour  
**Risk:** None (doesn't touch code)

**File:** `.github/workflows/build.yml`

```yaml
name: Build

on: [push, pull_request]

jobs:
  build-linux:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        qt: [5, 6]
    steps:
      - uses: actions/checkout@v4
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential autoconf automake libtool pkg-config \
            libmxml-dev libusb-1.0-0-dev zlib1g-dev \
            qt${{ matrix.qt }}-*-dev libv4l-dev \
            wine64-development-tools nsis \
            liblo-dev libopencv-dev
      
      - name: Generate configure
        run: autoreconf -fi
      
      - name: Configure
        run: ./configure
      
      - name: Build
        run: make -j$(nproc)
      
      - name: Check RPATH
        run: readelf -d src/.libs/ltr_server1 | grep -E "(RPATH|RUNPATH)"
```

**Benefits:**
- Catches build failures early
- Tests both Qt5 and Qt6
- Documents build dependencies

---

### 2.3 Replace Deprecated APIs

**Status:** 🔲 Not Started  
**Effort:** 1-2 hours  
**Risk:** Low (with testing)

| Old API | New API | Thread-Safe | Files |
|---------|---------|-------------|-------|
| `gethostbyname()` | `getaddrinfo()` | ✅ | Network code |
| `sprintf()` | `snprintf()` | Overflow-safe | Various |
| `strtok()` | `strtok_r()` | ✅ | Various |

---

## Tier 3: Higher Effort (Future)

### 3.1 C++11/14 Modernization

**Status:** 🔲 Not Started  
**Effort:** 4-8 hours  
**Risk:** Medium

**Scope:** C++ files only (`*.cpp`)

**Changes:**
```cpp
// pthread_mutex_t → std::mutex
#include <mutex>
std::mutex lock;
std::lock_guard<std::mutex> guard(lock);

// Raw pointers → smart pointers
std::unique_ptr<MyClass> ptr = std::make_unique<MyClass>();

// NULL → nullptr
void* p = nullptr;

// C-style casts → C++ casts
int x = static_cast<int>(y);
```

**Affected files:**
- `src/ltr_srv_master.cpp`
- `src/pref.cpp`
- `src/qt_gui/*.cpp`
- `src/mickey/*.cpp`

---

### 3.2 CMake Migration

**Status:** 🔲 Not Started  
**Effort:** 8-16 hours  
**Risk:** High (major build system change)

**Benefits:**
- Better IDE integration (CLion, VS Code)
- Cleaner syntax than autotools
- Native Windows support
- Modern dependency management

**Approach:**
1. Create `CMakeLists.txt` alongside existing autotools
2. Maintain both systems during transition
3. Eventually deprecate autotools

---

### 3.3 Unit Testing Framework

**Status:** 🔲 Not Started  
**Effort:** 4-8 hours  
**Risk:** None (additive)

**Options:**
- Google Test (gtest)
- Catch2
- Qt Test (for Qt components)

---

## Progress Tracking

| Task | Priority | Status | Est. Effort | Notes |
|------|----------|--------|-------------|-------|
| $ORIGIN RPATH | P1 | ✅ | 30 min | Relocatable builds (Fixed & extended to Wine Bridge) |
| Runtime validation | P1 | ✅ | 30 min | Better error messages |
| Format specifiers | P1 | ✅ | 15 min | Cleanup warnings |
| Qt6 support | P2 | ✅ | 4-6 hrs | Ported 3D view to Modern OpenGL/Shaders |
| GitHub Actions CI | P2 | ✅ | 1 hr | Automated testing |
| Deprecated APIs | P2 | 🔲 | 1-2 hrs | Thread safety |
| C++11/14 modernization | P3 | 🔲 | 4-8 hrs | Code quality |
| CMake migration | P3 | 🔲 | 8-16 hrs | Build system |
| Unit tests | P3 | 🔲 | 4-8 hrs | Code quality |

---

## Testing Checklist

After any modernization change, verify:

- [ ] `make distclean && ./configure && make` succeeds
- [ ] `ldd` shows no missing libraries
- [ ] RPATH points to correct location
- [ ] `ltr_gui` launches and can configure devices
- [ ] Tracking works in a test application
- [ ] Wine bridge works with a Windows game (if applicable)
- [ ] Both Qt5 and Qt6 build (after Qt6 migration)

---

## References

- Qt5 → Qt6 Porting Guide: https://doc.qt.io/qt-6/portingguide.html
- QRegularExpression: https://doc.qt.io/qt-6/qregularexpression.html
- CMake Tutorial: https://cmake.org/cmake/help/latest/guide/tutorial/
- GitHub Actions: https://docs.github.com/en/actions

---

*End of Modernization Roadmap*
