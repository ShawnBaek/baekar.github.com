# Implementation Plan: macOS Port of BackAR

**Branch**: `001-macos-port` | **Date**: 2026-02-08 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/001-macos-port/spec.md`

## Summary

Port BackAR (C++ Markerless AR Engine) from Windows/Visual Studio 2010/DirectX 9 to macOS using CMake, OpenGL, GLFW, and OpenCV 4.x. The port is organized into 5 sprints, each producing a PR that is verified compilable (and progressively runnable) on macOS. No refactoring or new features — only minimal changes necessary for platform compatibility.

## Technical Context

**Language/Version**: C++14 (minimum needed for OpenCV 4.x compatibility, supported by Apple Clang)
**Primary Dependencies**: OpenCV 4.x, GLFW 3.x, OpenGL (legacy profile), GLUT (freeglut), GLM (math library)
**Storage**: File-based (calibration data, YAML feature databases, image assets)
**Testing**: Manual verification — compile check + runtime AR functionality check per sprint
**Target Platform**: macOS 13+ (Ventura), Apple Silicon and Intel (universal)
**Project Type**: Single desktop application (C++ executable)
**Performance Goals**: Real-time camera feed at 30fps with AR overlay
**Constraints**: Must preserve original behavior; no architectural changes
**Scale/Scope**: ~83 C++ source files, ~10,000 lines of code to modify

## Constitution Check

*GATE: The constitution is a template with no project-specific principles defined. No gates to enforce. Pass.*

## Project Structure

### Documentation (this feature)

```text
specs/001-macos-port/
├── plan.md              # This file
├── research.md          # Phase 0 output (complete)
├── data-model.md        # Phase 1 output (complete)
├── quickstart.md        # Phase 1 output (complete)
└── tasks.md             # Phase 2 output (pending /speckit.tasks)
```

### Source Code (repository root)

```text
MarkerlessAR/                    # Main engine source (existing)
├── EngineMain.cpp               # Entry point — HEAVY MODIFICATION
├── d3d.cpp                      # DirectX rendering — REWRITE to OpenGL
├── wonjo.cpp / wonjo.h          # D3D math/rendering — REWRITE to OpenGL
├── cam.cpp                      # Camera matrix setup — MODIFY for OpenGL
├── External_Include.h           # Global state — MODIFY threading types
├── stdafx.h                     # Precompiled header — MODIFY includes
├── global_include.h             # Global includes — MODIFY pragmas/includes
├── BaekAR.cpp/.hpp              # Core AR — MODIFY OpenCV API
├── Detector.cpp/.hpp            # Feature detection — MODIFY OpenCV API
├── SungwookFeature.cpp/.hpp     # Custom features — MODIFY OpenCV API
├── SungwookAR.hpp               # AR utilities — MODIFY platform types
├── Contents.cpp/.hpp            # Content mgmt — MINOR changes
├── brisk/                       # BRISK descriptor — MODIFY alignment macros
│   ├── brisk.h / brisk.cpp
│   ├── hammingsse.hpp           # SIMD alignment — MODIFY __declspec
│   └── Matcher.h                # SIMD alignment — MODIFY __declspec
├── agast/                       # AGAST detector — NO CHANGES (clean)
├── calibration/                 # Camera calibration — MODIFY OpenCV API
│   ├── Camera.h
│   ├── BaekARCamera.h/.cpp      # HEAVY MODIFICATION (70+ CvMat allocs)
│   ├── cvFindExtrinsicCameraParams3.h/.cpp  # MODIFY to cv::solvePnP
│   └── KatoPoseEstimation/      # MODIFY OpenCV API (CvPoint/CvMat)
├── HandyAR/                     # Hand tracking — MODIFY OpenCV API
│   ├── FingertipPoseEstimation.cpp/.h
│   ├── FingertipTracker.cpp/.h
│   ├── HandRegion.cpp/.h
│   ├── MixGaussian.cpp/.h
│   └── ColorDistribution.cpp/.h
├── image/                       # Reference images (unchanged)
├── calibration/                 # Calibration data (unchanged)
├── database/                    # Feature databases (unchanged)
└── 3dobjects/                   # 3D models (unchanged)

CMakeLists.txt                   # NEW — build system (root)
README.md                        # MODIFY — build instructions
```

**Structure Decision**: Preserve existing source layout entirely. Add only CMakeLists.txt at root and modify existing files in-place.

## Sprint Plan

### Sprint 1: Build System + MSVC Cleanup (PR: Compilable skeleton)

**Goal**: Create CMake build system, eliminate all MSVC-specific code, stub out DirectX/Win32 dependencies so the project compiles on macOS (with empty rendering stubs).

**Files to create**:
- `CMakeLists.txt` — CMake build configuration finding OpenCV, GLFW, OpenGL

**Files to modify**:
- `global_include.h` — Remove `#pragma warning`, `#pragma comment(lib,...)`, D3D includes
- `stdafx.h` — Remove `<atlstr.h>`, add macOS-compatible includes
- `External_Include.h` — Replace `<windows.h>`, `<process.h>` with `<thread>`, `<mutex>`; replace HANDLE/CRITICAL_SECTION types with std::thread/std::mutex; fix GLUT include path
- `SungwookAR.hpp` — Remove `<atlstr.h>`, `__stdcall`
- `brisk/brisk.h` — Remove `#pragma comment(lib,...)`, fix `__forceinline` → `inline`, fix `__declspec(align(16))` → `alignas(16)`
- `brisk/hammingsse.hpp` — Fix `__declspec(align(16))` → `alignas(16)`
- `brisk/Matcher.h` — Fix `__declspec(align(16))` → `alignas(16)`
- `d3d.cpp` — Replace `stdext::hash_map` → `std::unordered_map`; stub all D3D functions
- `wonjo.h` — Replace D3D type declarations with OpenGL/float equivalents (stubs)
- `wonjo.cpp` — Remove `#pragma comment(lib,"glut32.lib")`, stub D3D functions
- `EngineMain.cpp` — Remove `<windows.h>`, stub WinMain → main(), comment out D3D calls
- `cam.cpp` — Stub D3D matrix functions

**Verification**: `cmake . && make` succeeds with zero compilation errors (rendering is no-op stubs).

---

### Sprint 2: OpenCV 4.x Migration (PR: Compilable with modern OpenCV)

**Goal**: Migrate all 537+ legacy OpenCV C API calls to modern C++ API so the project compiles with OpenCV 4.x from Homebrew.

**Files to modify (by priority)**:

**High-effort files**:
- `calibration/BaekARCamera.cpp` (1185 lines) — Migrate 70+ `cvCreateMat()` → `cv::Mat()`, `CvPoint3D64f` → `cv::Point3d`, `CvPoint2D32f` → `cv::Point2f`, `cvmGet/cvmSet` → `mat.at<>()`
- `calibration/BaekARCamera.h` — Update member types: `CvMat*` → `cv::Mat`
- `calibration/Camera.h` — Update member types: `CvMat*` intrinsic/distortion/rotation/translation → `cv::Mat`
- `KatoPoseEstimation/KatoPoseEstimator.cpp` (833 lines) — Migrate CvPoint/CvMat usage
- `KatoPoseEstimation/KatoPoseEstimator.h` — Update CvPoint2D/CvPoint3D types
- `calibration/cvFindExtrinsicCameraParams3.cpp` (246 lines) — Replace with `cv::solvePnP()` or migrate CvMat usage

**Medium-effort files**:
- `HandyAR/MixGaussian.cpp` — Migrate cvCreateMat usage
- `HandyAR/PoseEstimation.cpp` — Migrate CvMat operations
- `HandyAR/ColorDistribution.cpp` — Migrate cvCreateMat usage
- `HandyAR/FingerTip.cpp` — Migrate CvMat/CvPoint
- `HandyAR/FingertipPoseEstimation.cpp` — Migrate CvMat
- `HandyAR/FingertipTracker.cpp` — Migrate CvMat
- `HandyAR/HandRegion.cpp` — Migrate CvMat

**Low-effort files**:
- `EngineMain.cpp` — Replace `IplImage`/`CvCapture` globals with cv::Mat/cv::VideoCapture
- `External_Include.h` — Remove `IplImage`, `CvCapture` types; remove GPU module include
- `BaekAR.cpp/.hpp` — Update any remaining legacy types
- `Detector.cpp/.hpp` — Update any remaining legacy types
- `SungwookFeature.cpp/.hpp` — Update any remaining legacy types
- `brisk/projection.h` — Migrate IplImage/CvMat usage
- AGAST files — Update CvPoint usage if any (minimal)

**Verification**: `cmake . && make` succeeds with OpenCV 4.x. Feature detection logic compiles.

---

### Sprint 3: Threading + GLFW Windowing (PR: Compilable, window opens)

**Goal**: Replace Win32 threading with std::thread, replace Win32 windowing with GLFW. Application launches and opens a window.

**Files to modify**:

**Threading replacement**:
- `EngineMain.cpp`:
  - Replace `_beginthreadex()` calls → `std::thread` construction
  - Replace `_endthreadex()` → thread function returns naturally
  - Replace `CRITICAL_SECTION` usage → `std::mutex` with `std::lock_guard`
  - Replace `CreateMutex()` → `std::mutex` member
  - Remove `__stdcall` from thread function signatures
  - Replace `HANDLE` thread handles → `std::thread` objects
- `External_Include.h`:
  - Replace `HANDLE hMatchingThread[2]` → `std::thread matchingThread[2]`
  - Replace `HANDLE hTrackingThread[2]` → `std::thread trackingThread[2]`
  - Replace `CRITICAL_SECTION` → `std::mutex`
  - Replace `unsigned __stdcall` function signatures → `void` functions

**Windowing replacement**:
- `EngineMain.cpp`:
  - Replace `WinMain()` → `int main()`
  - Replace `RegisterClass/CreateWindow` → `glfwInit/glfwCreateWindow`
  - Replace message loop (`PeekMessage/DispatchMessage`) → `glfwPollEvents()` loop
  - Replace `WndProc` → GLFW key/mouse callbacks
  - Remove `AllocConsole()` (not needed on macOS)
  - Replace `ExitProcess()` → `exit()` or clean return
  - Replace `GetKeyState(VK_LBUTTON)` → GLFW mouse button state
  - Replace `GetCursorPos/ScreenToClient` → GLFW cursor position callback

**Verification**: Application compiles, launches, opens a GLFW window (blank/black). Threads start without crashes.

---

### Sprint 4: OpenGL Rendering (PR: Camera feed visible, basic AR overlay)

**Goal**: Replace all DirectX 9 rendering with OpenGL. Camera feed displays in window. Basic 3D rendering works.

**Files to rewrite/heavily modify**:

**d3d.cpp → opengl_renderer.cpp** (or modify in-place):
- Replace `AAR3DInitD3D()` → OpenGL context setup (already created by GLFW)
- Replace `BeginRender()` → `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`
- Replace `Flip()` → `glfwSwapBuffers()`
- Replace D3D texture creation → `glGenTextures/glTexImage2D` for camera preview
- Replace `AAR3DDrawCameraPreview()` → Draw textured quad with camera frame data
- Replace vertex buffer creation → OpenGL vertex arrays or immediate mode
- Replace `D3DXLoadMeshFromX()` → Simple OBJ loader or use bunny.h vertex data
- Replace `DrawMesh()` → `glDrawArrays()` or `glBegin/glEnd`
- Replace D3D render states → `glEnable/glDisable` equivalents
- Replace D3D lighting → `glLightfv/glMaterialfv`
- Replace `stdext::hash_map` mesh cache → `std::unordered_map`

**wonjo.cpp/.h**:
- Replace `D3DXMATRIXA16` → `float[16]` arrays or GLM mat4
- Replace `SetProjectionMatrix()` → `glMatrixMode(GL_PROJECTION); glLoadMatrixf()`
- Replace `SetModelViewMatrix()` → `glMatrixMode(GL_MODELVIEW); glLoadMatrixf()`
- Replace D3DX math functions → GLM or manual implementations
- Replace picking code → OpenGL unproject (`gluUnProject`)
- Remove screen capture code (Win32 GDI PrintWindow) — stub or remove

**cam.cpp**:
- Replace `D3DXMATRIXA16` → `float[16]`
- Replace `GetDevice()->SetTransform()` → `glLoadMatrixf()`

**EngineMain.cpp**:
- Update `mainLoop()` to call OpenGL rendering instead of wonjo_dx:: namespace
- Wire up camera frame → OpenGL texture upload → display
- Wire up pose estimation → OpenGL modelview matrix → 3D overlay

**Verification**: Application opens window, displays live camera feed. When pointed at reference image, basic 3D geometry appears.

---

### Sprint 5: AR Integration + Polish + README (PR: Fully functional)

**Goal**: Ensure complete AR pipeline works end-to-end. Fix runtime issues. Update README.

**Tasks**:
- **Camera access**: Verify `cv::VideoCapture(0)` works on macOS (AVFoundation backend)
- **Feature matching**: Verify BRISK/AGAST detect features correctly with OpenCV 4.x
- **Pose estimation**: Verify Kato pose estimator produces correct transformations
- **3D overlay**: Verify rendered objects align with detected markers
- **Hand tracking**: Verify HandyAR module works (if possible — may need skin detection tuning)
- **Stability**: Test 5+ minutes of continuous operation
- **Asset paths**: Ensure all relative paths to image/, calibration/, database/ resolve correctly at runtime
- **Edge cases**: Handle no camera, missing files, permission errors gracefully
- **README.md**: Write comprehensive build instructions per quickstart.md
- **Cleanup**: Remove any remaining Windows-only dead code, build artifacts (.sdf, .suo, .tlog files)

**Verification**: Full AR functionality — camera feed, feature detection, pose estimation, 3D overlay. README enables fresh build in <15 minutes.

## Complexity Tracking

| Decision | Why Needed | Simpler Alternative Rejected Because |
|----------|------------|-------------------------------------|
| GLFW for windowing | Need cross-platform window + OpenGL context | Raw Cocoa/NSWindow too platform-specific; SDL2 heavier than needed |
| Legacy OpenGL (not modern) | Original code uses fixed-function pipeline (matrix stack, immediate mode) | Modern OpenGL (3.3+ core profile) would require complete shader rewrite — out of scope |
| GLM for D3DX math replacement | Need D3DXMatrixScaling, D3DXMatrixInverse, etc. equivalents | Manual float[16] math is error-prone for complex operations like picking |
| In-place file modification | Preserve git history, minimize diff noise | New files would duplicate structure and lose authorship history |

## Risk Register

| Risk | Impact | Mitigation |
|------|--------|------------|
| OpenCV C API migration introduces bugs | HIGH | Compare numerical outputs before/after for key functions (projection, pose) |
| macOS OpenGL deprecation | LOW | Legacy profile still works on macOS 13+; warnings are cosmetic |
| Camera permissions block testing | MEDIUM | Document Info.plist or entitlement needs; test with video file fallback |
| Hand tracking (skin detection) fails on macOS | MEDIUM | HandyAR is P3 priority; can defer if core AR works |
| .X mesh format not loadable | LOW | Use bunny.h embedded data; use existing .obj files for iPad model |
| SSE intrinsics on Apple Silicon | LOW | Rosetta 2 handles SSE→NEON translation; native ARM build may need NEON port |
