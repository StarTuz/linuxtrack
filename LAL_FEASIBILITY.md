# LAL (Licensed Asset Loader) Feasibility Study

**Date:** 2025-12-26
**Target:** Linuxtrack Fork (StarTuz)
**Author:** Antigravity AI Assistant

## 1. Executive Summary

**Feasibility:** **HIGH**
**Effort:** Medium (Architectural Refactoring + UI work)
**Recommendation:** Proceed. It will significantly improve user experience and legal compliance by removing the dependency on "hacky" Wine-based extraction and providing a unified interface for proprietary assets.

**Strategic Impact:** LAL positions Linuxtrack as a "neutral ground" for hardware vendors (e.g., Tobii, NaturalPoint) to support Linux via officially sanctioned binary packages without open-sourcing their core IP.

The "Authorized/Licensed Asset Loader" (LAL) would replace the current hardcoded `TirFwExtractor` and `Mfc42uExtractor` classes with a **data-driven, generalized system** to manage proprietary dependencies (firmware, DLLs, game data).

---

## 2. Problem Statement

Currently, Linuxtrack requires several proprietary files to function fully:
1.  **TrackIR Firmware:** Extracted from NaturalPoint installers.
2.  **MFC42.dll / TIRViews.dll:** Required for the Wine bridge to talk to games.

The current solution (`src/qt_gui/extractor.cpp`) is:
*   **Brittle:** Hardcoded logic for finding specific C++ hashes inside files.
*   **Heavy:** Often defaults to running `wine` to execute the installer (though a `7z` fallback exists).
*   **Opaque:** No clear "manifest" of what is installed vs. missing.
*   **Unmaintained:** Relies on legacy `spec.txt` and generic "poems" (signatures).

---

## 3. Proposed Solution: Project LAL

Create a new module, `LAL` (Licensed Asset Loader), that standardizes this process.

### 3.1 Core Principles
1.  **No Redistribution:** The repo contains NO proprietary data.
2.  **Native First:** Use Linux-native tools (`7z`, `cabextract`, `unshield`, `isolyzer`) to extract files. **Eliminate Wine runtime dependency for installation.**
3.  **Data-Driven:** Define assets in a JSON manifest, not C++ code.
4.  **Verifiable:** Strong SHA-256 hashing of installers and extracted artifacts.
5.  **Vendor Friendly:** Allow vendors to provide a simple ZIP/TAR of their Linux binaries (blob) which LAL verifies and installs, bypassing the "extract from Windows installer" step entirely.

### 3.2 Architecture

#### A. The Manifest (`lal_manifest.json`)
A JSON file shipped with `ltr_gui` describing known assets.

```json
{
  "assets": [
    {
      "id": "tir_firmware_v5",
      "name": "TrackIR 5 Firmware",
      "version": "5.4.2",
      "description": "Required for device initialization.",
      "license_type": "Proprietary",
      "sources": [
        {
          "type": "url",
          "url": "https://naturalpoint.com/files/...",
          "sha256": "a3f..."
        }
      ],
      "extraction": {
        "tool": "7z",
        "map": {
          "Drivers/USB/sgl.dat": "firmware.bin",
          "NPClient.dll": "NPClient.dll"
        }
      }
    },
    {
      "id": "tobii_et5_blob",
      "name": "Tobii Eye Tracker 5 (Proprietary Driver)",
      "version": "1.0.0-linux-beta",
      "vendor": "Tobii",
      "license_type": "Proprietary (Blob)",
      "sources": [{ "type": "vendor_portal", "url": "https://tobii.com/linux/et5_blob.tar.gz" }],
      "install_path": "~/.local/share/linuxtrack/drivers/tobii/"
    }
  ]
}
```

#### B. The Manager (`LALManager` C++ Class)
*   **`loadManifest(path)`**: Parses the JSON.
*   **`status(assetID)`**: Returns `INSTALLED`, `MISSING`, `CORRUPT`.
*   **`install(assetID, path_to_installer)`**:
    1.  Verifies hash of `path_to_installer`.
    2.  Creates safe temp dir.
    3.  Runs `7z e ...` (or appropriate tool).
    4.  Moves mapped files to `~/.local/share/linuxtrack/lal/<assetID>/`.
    5.  Verifies output hashes.

#### C. The GUI (`LALDialog`)
A clean list view:
*   ✅ **TrackIR Firmware**: Installed
*   ❌ **MFC42 Helper**: Missing (Download Interface generic button)
*   ❌ **Tobii Driver**: Missing (Vendor Link)

---

## 4. Strategic Advantage: Vendor Partnership

Hardware manufacturers (e.g., Tobii, NaturalPoint) face increasing pressure to support Linux (Steam Deck, gaming market growth) but are often hesitant to open-source their core IP (processing algorithms, precise device handshakes).

**LAL offers a compromise:**
1.  **"Blob" Support:** Vendors can release a closed-source binary driver (blob) for Linux.
2.  **Standard Delivery:** LAL handles the download, verification, installation, and path management.
3.  **Legal Safety:** The open-source project (Linuxtrack) never redistributes the code. The user downloads it directly from the vendor via LAL's interface.
4.  **No "Hack" Stigma:** Instead of telling users "find a script on GitHub to extract files from a Windows EXE," vendors can say "Install Linuxtrack and click 'Install Tobii Driver'".

*Example:* Tobii Gaming devices (Eye Tracker 5) currently lack official Linux support. Community workarounds exist but are fragmented. LAL could provide a stable, "officially sanctionable" target for Tobii to ship a simple Linux binary blob.

---

## 5. Safety & Preservation Strategy

**Goal:** Innovate without breaking the solid foundation we have built (Surgical Injection, TrackIR reliability).

### 5.1 Parallel Existence (Phase 1)
*   **Do Not Delete:** The legacy `extractor.cpp` and `plugin_install.cpp` logic will remain untouched initially.
*   **Opt-In:** LAL will be added as a "Beta Features" tab or a separate "Asset Manager" dialog.
*   **Fallback:** If LAL fails to extract (e.g., `7z` missing, installer format changed), the system explicitly falls back to the legacy code.

### 5.2 Non-Destructive Integration (Phase 2)
*   **Wrappers:** The new `LALManager` can call the legacy code internally if a manifest entry is marked `legacy_method: true`.
*   **Verification-Only:** LAL can initially just *verify* that the files extracted by the legacy system are correct, providing better diagnostics without taking over the extraction logic risk.

### 5.3 Full Replacement (Phase 3 - Future)
*   Only after LAL has been proven robust across multiple distros (Arch, Debian, Fedora) and device types will the legacy code be deprecated.

---

## 6. Implementation Steps

### Phase 1: Prototype (Discovery)
1.  Verify `7z` (p7zip) can consistently extract `TrackIR_5.4.2.exe` without Wine. (Confirmed in current `extractor.cpp` logic).
2.  Draft the `lal_manifest.json` for existing assets.

### Phase 2: Core Logic (C++)
1.  Add `nlohmann/json` dependency (single header) for manifest parsing.
2.  Implement `LALManager` class to replace `TirFwExtractor`.
3.  Implement native process execution for `7z`.

### Phase 3: GUI Integration
1.  Replace the "Install Firmware" wizard in `plugin_install.cpp` with the new LAL Dashboard.
2.  Update the "Wine Bridge" to verify assets exist in the LAL storage location before "Surgical Injection".

### Phase 4: Legacy Removal
1.  Delete `extractor.cpp`, `extractor.h`, `extractor.ui`.
2.  Remove `WineLauncher` dependency from the extraction process (WinLauncher remains for *launching* games, but not installing firmware).

---

## 7. Conclusion

Implementing LAL is the correct next step for a "modern" Linuxtrack. It aligns with the goal of **removing fragility** and **improving the user experience**. It turns a "hacky script" effectively into a "Package Manager for Proprietary Drivers".

**Status:** APPROVED for future roadmap.
