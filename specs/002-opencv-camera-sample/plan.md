# Implementation Plan: Sample OpenCV Camera Project on macOS

**Branch**: `002-opencv-camera-sample` | **Date**: 2026-02-14 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/002-opencv-camera-sample/spec.md`

## Summary

Create a minimal, standalone C++ sample project that demonstrates how to correctly open and display a live camera feed using OpenCV on macOS. The project applies all proven macOS AVFoundation workarounds (env var, Info.plist, timeout, error handling) in a single-file reference implementation. Built with CMake, depends only on OpenCV 4.x from Homebrew.

## Technical Context

**Language/Version**: C++14 (matches BackAR, minimum for OpenCV 4.x with Apple Clang)
**Primary Dependencies**: OpenCV 4.x (Homebrew)
**Storage**: N/A (no persistent data)
**Testing**: Manual verification — build, launch, verify camera feed displays
**Target Platform**: macOS 13+ (Ventura), Apple Silicon and Intel
**Project Type**: Single desktop application (minimal sample)
**Performance Goals**: Real-time camera feed at native capture rate (typically 30 fps)
**Constraints**: Single source file + CMakeLists.txt + Info.plist for maximum simplicity
**Scale/Scope**: ~150 lines of C++, 4 files total

## Constitution Check

*GATE: The constitution is a template with no project-specific principles defined. No gates to enforce. Pass.*

## Project Structure

### Documentation (this feature)

```text
specs/002-opencv-camera-sample/
├── plan.md              # This file
├── research.md          # Phase 0 output (complete)
├── data-model.md        # Phase 1 output (complete)
├── quickstart.md        # Phase 1 output (complete)
└── tasks.md             # Phase 2 output (pending /speckit.tasks)
```

### Source Code (repository root)

```text
samples/opencv-camera/
├── main.cpp             # Single source file — all camera logic
├── CMakeLists.txt       # Build configuration (find OpenCV, macOS bundle)
├── Info.plist           # macOS camera permission + Continuity Camera keys
└── README.md            # Build & run instructions (from quickstart.md)
```

**Structure Decision**: Flat single-directory layout under `samples/opencv-camera/`. This keeps the sample self-contained and separate from the main BackAR source tree. No subdirectories needed — the entire project is 4 files.

## Implementation Plan

### Phase 1: Build System + Info.plist

**Goal**: CMakeLists.txt and Info.plist that configure a macOS app bundle finding OpenCV.

**Files to create**:
- `samples/opencv-camera/CMakeLists.txt` — cmake_minimum_required(3.20), find_package(OpenCV 4 REQUIRED), C++14, MACOSX_BUNDLE with Info.plist
- `samples/opencv-camera/Info.plist` — NSCameraUsageDescription, NSCameraUseContinuityCameraDeviceType, bundle metadata

**Verification**: `cmake .` succeeds in a build directory (source won't compile yet).

---

### Phase 2: Camera Open with macOS Workarounds

**Goal**: main.cpp that opens the camera with all AVFoundation workarounds.

**Key implementation details** (proven in BackAR):

1. **Set env var first**: `setenv("OPENCV_AVFOUNDATION_SKIP_AUTH", "1", 1)` before any OpenCV call
2. **Timeout wrapper**: Open `cv::VideoCapture(0)` in a detached `std::thread` with `std::atomic<bool>` signaling; poll with 100ms sleep; timeout after 10 seconds
3. **Error diagnostics**: If `!cap.isOpened()` after timeout, print actionable message pointing to System Settings > Privacy & Security > Camera
4. **Resolution/FPS overlay**: Read `CAP_PROP_FRAME_WIDTH`, `CAP_PROP_FRAME_HEIGHT`, compute FPS from frame timing, display in window title via `cv::setWindowTitle()`

**Verification**: Binary launches, opens camera, displays feed. ESC/Q quits cleanly.

---

### Phase 3: Error Handling + README

**Goal**: Graceful handling of all failure modes plus documentation.

**Error scenarios**:
- Camera permission denied → print System Settings path, exit with code 1
- No camera hardware → print "no camera found", exit with code 1
- Camera daemon stuck → timeout after 10s, print daemon recovery commands, exit with code 1
- OpenCV not installed → CMake fails with clear `find_package` error

**Files to create**:
- `samples/opencv-camera/README.md` — Prerequisites, install deps, build, run, troubleshooting

**Verification**: Full end-to-end: fresh build, camera opens, ESC quits, error messages work.

## Complexity Tracking

| Decision | Why Needed | Simpler Alternative Rejected Because |
|----------|------------|-------------------------------------|
| cv::imshow for display | Minimal dependency — only needs OpenCV | GLFW would add a second dependency for a display-only sample |
| macOS .app bundle | Required for TCC camera permission dialog | Bare executable may silently fail to get camera access |
| Detached thread timeout | Prevents indefinite hang on stuck camera daemon | Synchronous open can hang forever with no recovery |

## Risk Register

| Risk | Impact | Mitigation |
|------|--------|------------|
| cv::imshow may not work reliably on macOS without a run loop | MEDIUM | Tested in OpenCV 4.x — HighGUI uses Cocoa backend on macOS which creates its own run loop. Fallback: switch to GLFW if needed. |
| Camera permission not granted to non-bundled executable | HIGH | Build as .app bundle via MACOSX_BUNDLE. Document `open` command for first launch. |
| Continuity Camera becomes default on macOS 15+ | MEDIUM | Info.plist includes NSCameraUseContinuityCameraDeviceType key |
