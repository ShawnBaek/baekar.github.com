# Research: Sample OpenCV Camera Project on macOS

**Feature**: 002-opencv-camera-sample
**Date**: 2026-02-14

## Research Topics

### 1. macOS AVFoundation Camera Workarounds

**Decision**: Use the proven workaround pattern from BackAR: set `OPENCV_AVFOUNDATION_SKIP_AUTH=1` env var before any VideoCapture usage.

**Rationale**: OpenCV's AVFoundation backend has a known blocking issue (#7519) where `cv::VideoCapture::open()` hangs waiting for authorization that never completes when the app doesn't have a proper run loop or Info.plist. Setting this env var tells OpenCV to skip its internal auth handling, deferring to the macOS system dialog triggered by the Info.plist `NSCameraUsageDescription` key.

**Alternatives considered**:
- Using `cv::CAP_V4L2` backend — not available on macOS
- Using GStreamer backend — adds heavy dependency for a sample project
- Not setting the env var and relying on OpenCV defaults — causes indefinite hang

### 2. Info.plist Requirements for macOS Camera

**Decision**: Include Info.plist with `NSCameraUsageDescription` and `NSCameraUseContinuityCameraDeviceType` keys.

**Rationale**: macOS requires `NSCameraUsageDescription` for the TCC (Transparency, Consent, and Control) privacy dialog. macOS 15+ requires `NSCameraUseContinuityCameraDeviceType` to properly handle Continuity Camera as the default device (OpenCV issues #26305, #26371).

**Alternatives considered**:
- Running without Info.plist — camera permission never granted, silent failure
- Using only NSCameraUsageDescription without Continuity key — fails on macOS 15+

### 3. Camera Open Timeout Strategy

**Decision**: Open VideoCapture in a detached std::thread with 10-second polling timeout using std::atomic<bool> for signaling.

**Rationale**: Proven in BackAR's Capture.cpp. The camera daemon can hang indefinitely on macOS (daemon crash, beta bugs). A detached thread with timeout ensures the main thread remains responsive and can report the issue.

**Alternatives considered**:
- Synchronous open with no timeout — hangs indefinitely if daemon stuck
- POSIX signal-based timeout (alarm/SIGALRM) — not safe with C++ objects
- std::future with wait_for — viable but detached thread is simpler and proven

### 4. Display Method: cv::imshow vs GLFW

**Decision**: Use `cv::imshow()` (OpenCV HighGUI) for this sample project.

**Rationale**: This is a minimal sample — adding GLFW would introduce an extra dependency that defeats the "single dependency" simplicity goal. BackAR uses GLFW because it needs OpenGL rendering, but this sample only needs to display a camera frame. `cv::imshow` is built into OpenCV and requires zero additional libraries.

**Alternatives considered**:
- GLFW + OpenGL texture — overkill for a camera display sample, adds dependency
- SDL2 — adds dependency
- Native Cocoa NSWindow — not cross-platform, complex for a sample

### 5. Build System

**Decision**: CMake with cmake_minimum_required(3.20), find_package(OpenCV 4 REQUIRED), C++14 standard.

**Rationale**: Matches BackAR's proven CMake configuration. CMake 3.20 is widely available via Homebrew. C++14 is the minimum for OpenCV 4.x with Apple Clang.

**Alternatives considered**:
- Plain Makefile with pkg-config — less portable, harder to configure macOS bundle
- Meson — less common, smaller community
- Direct clang++ command — no Info.plist/bundle support

### 6. macOS App Bundle for Camera Permissions

**Decision**: Build as a macOS .app bundle via CMake's MACOSX_BUNDLE property so Info.plist is embedded.

**Rationale**: macOS TCC (privacy framework) only grants camera permission to app bundles that include an Info.plist with NSCameraUsageDescription. A bare executable without a bundle may silently fail to access the camera. CMake's MACOSX_BUNDLE property handles this automatically.

**Alternatives considered**:
- Bare executable without bundle — camera permission may not be granted
- Manual .app bundle creation via post-build script — unnecessary when CMake handles it
