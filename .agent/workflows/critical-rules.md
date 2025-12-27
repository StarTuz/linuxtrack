---
description: Critical rules for working on linuxtrackfixed
---

# CRITICAL RULES - READ BEFORE ANY WORK

## Project Map

| Folder | Description | Access Rules |
|--------|-------------|--------------|
| `/home/startux/Code/linuxtrackfixed/` | Legacy linuxtrack with modern fixes | CONSTRAINED - follow rules below |
| `/home/startux/Code/tuxtracksold/` | Old modernization attempt (wine-bridge broken) | EXPLICIT PERMISSION ONLY |
| `/home/startux/Code/tuxtracks/` | NEW greenfield project | Different rules - see its own workflows |

## Shared Documentation

- **`/home/startux/Code/HEAD_TRACKING_PROTOCOL.md`** - Protocol specs both projects must follow

---

## DO NOT ACCESS OTHER REPOSITORIES (for linuxtrackfixed work)

1. **DO NOT** access `/home/startux/Code/tuxtracksold/` unless the user EXPLICITLY instructs you to
2. **NEVER** run `git checkout HEAD` or any git command that reverts to upstream
3. **NEVER** run `git reset` without explicit user permission
4. The upstream remote (uglyDwarf/linuxtrack) contains OLD code - our fixes are LOCAL and UNCOMMITTED

## WHY THIS MATTERS

This repository (`linuxtrackfixed`) contains uncommitted modifications on top of the upstream uglyDwarf/linuxtrack code. Running `git checkout` or similar commands will DESTROY these local fixes.

## BEFORE MODIFYING ANY FILE

1. Read ALL documentation: HANDOFF.md, COMPILATION_FIXES.md, INSTALLATION_PATH_ANALYSIS.md, MODERNIZATION_ROADMAP.md
2. Follow the MODERNIZATION_ROADMAP.md task order
3. ASK before reverting or restoring any file
4. If a file appears corrupted, ASK the user - do not assume and do not auto-fix

## GIT COMMANDS THAT REQUIRE EXPLICIT PERMISSION

- `git checkout`
- `git reset`
- `git restore`
- `git revert`
- `git stash`
- Any command that modifies the working tree from git history

---

## About tuxtracksold

This is an OLD modernization attempt - the first aggressive effort to modernize linuxtrack.

**What Worked:**
- Qt6 compatibility
- Tracking window with camera view and 3D view (modernized for Wayland, backwards compatible with X11)
- X-Plane integration improvements

**What Failed:** Wine bridge never worked

**Status:** Archived/Reference only. Contains useful code snippets and approaches that may be referenced for the new project. Only access when the user explicitly says to look there for reference.

## About tuxtracks (new)

This is a completely NEW greenfield implementation. It has its own workflow rules and is not constrained by linuxtrack legacy issues.
