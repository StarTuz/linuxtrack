# Linuxtrack Modernized (Proton & Qt6 Edition)

[![CI/CD Build](https://github.com/StarTuz/linuxtrack/actions/workflows/build-release.yml/badge.svg)](https://github.com/StarTuz/linuxtrack/actions)

This is a modernized fork of the [Linuxtrack](https://github.com/uglyDwarf/linuxtrack) project, updated to support modern Linux desktop standards (Qt6, Wayland, Modern OpenGL) and specifically optimized for **Proton/Steam Deck** environments.

## 🚀 Key Modernizations

Unlike other legacy forks, this version addresses deep technical debt to ensure long-term stability:

- **Native Firmware Manager (LAL)**: New "Manage Assets" GUI allows legal, drag-and-drop installation of proprietary TrackIR/SmartNav firmware without needing Wine or external tools.
- **Surgical Wine Injection**: No more fragile `.exe` installers! Directly inject bridge DLLs into Wine/Proton prefixes with one click.
- **Proton-Aware Environment**: Automatically configures `PROTON_NO_FSYNC` and `PROTON_NO_ESYNC` to prevent race conditions and tracking stutters in games like *Elite Dangerous*.
- **Qt6 & Wayland Support**: Fully ported from Qt4/5 to Qt6. Removed `QtX11Extras` to allow the GUI to run natively on Wayland.
- **Modern OpenGL (3.0+)**: Rewrote the 3D Tracking View using a programmable shader-based pipeline (GLES 3.0), replacing the broken 20-year-old fixed-function legacy code.
- **Bison/Flex Removal**: Replaced complex legacy configuration parsers with the lightweight, robust `mINI` library.
- **TrackIR 5 V3 Support**: Full hardware activation for the latest TrackIR 5 revisions (using optimized 0x1A/0x1B initialization packets).

## 🎮 Verified Games & Apps

- ✅ **DCS World** (Proton, with Controller.exe hotkeys)
- ✅ **Elite Dangerous** (Proton 10/Steam Launcher)
- ✅ **X-Plane 12** (Native Linux Plugin)
- ✅ **X4 Foundations** (Via ltr_udp)

## 🛠️ Installation

### 1. Build from Source (CMake)
**Prerequisites:** Qt6 (likely 6.10+ recommended), OpenGLES/OpenGL drivers, libusb-1.0, libmxml, liblo.

```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make -j$(nproc)
sudo make install
```

### 2. AppImage Distribution
**Generic AppImage:** The generic CI build is currently experimental due to Qt6 ABI complexity.
**Arch Linux AppImage:** A verified, working AppImage (built against Arch system Qt) is available. This version uses the host system's Qt libraries for maximum compatibility on rolling release distros.


### 3. Configure Wine/Proton Support
1. Open `ltr_gui`.
2. Go to the **Misc.** tab.
3. Click **"Install Linuxtrack-Wine support..."**.
4. Select your game's Wine prefix from the auto-discovered list.
5. Controller.exe is automatically installed for Pause/Recenter hotkeys.

## 🗺️ Roadmap & Next Steps

Current progress is tracked in [MODERNIZATION_ROADMAP.md](MODERNIZATION_ROADMAP.md).

**Completed:**
- [x] Qt6 Migration & Wayland safety.
- [x] Modern OpenGL Shader-based 3D View.
- [x] Bison/Flex to mINI migration.
- [x] Surgical Wine/Proton Bridge Injection.
- [x] Project LAL (Native Firmware Manager) Phase 3 Integration.
- [x] Automatic Steam/Proton/Lutris/Bottles Prefix Discovery.
- [x] Controller.exe for customizable Pause/Recenter hotkeys.

**Upcoming:**
- [ ] Native UI for model scaling/offsets.
- [-] AppImage/Flatpak distribution (Arch AppImage verified).
- [ ] Linux-native global hotkey daemon.

---

*For detailed technical implementation notes, see [HANDOFF.md](HANDOFF.md).*
