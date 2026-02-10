# Tasks: macOS Port of BackAR

**Input**: Design documents from `/specs/001-macos-port/`
**Prerequisites**: plan.md (required), spec.md (required), research.md, data-model.md, quickstart.md

**Tests**: Not requested — manual verification per sprint (compile check + runtime check).

**Organization**: Tasks are grouped by user story to enable independent implementation. Each sprint produces a PR verified on macOS.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Build System)

**Purpose**: Create CMake build system so the project can be built on macOS. This is the skeleton that everything else depends on.

- [ ] T001 Create CMakeLists.txt at repository root with cmake_minimum_required(3.20), find_package for OpenCV, GLFW3, OpenGL, GLUT, and GLM; add all MarkerlessAR/*.cpp source files; set C++14 standard; configure include directories and link libraries
- [ ] T002 Add .gitignore entries for build/, *.o, CMakeCache.txt, cmake_install.cmake, Makefile, and macOS .DS_Store files

**Checkpoint**: CMake configuration runs (`cmake .`) even though sources don't compile yet.

---

## Phase 2: Foundational (MSVC Cleanup — Blocking Prerequisites)

**Purpose**: Remove all MSVC-specific code so sources can compile with Apple Clang. This MUST complete before any user story work.

**Sprint 1 PR scope**: Phase 1 + Phase 2 together = compilable skeleton on macOS.

### Global Headers & Includes

- [ ] T003 [P] Clean up MarkerlessAR/global_include.h — remove all `#pragma warning(disable:...)` directives, remove all `#pragma comment(lib,...)` directives for dxguid/d3d9/d3dx9, remove DirectX includes
- [ ] T004 [P] Clean up MarkerlessAR/stdafx.h — remove `#include <atlstr.h>`, remove `#include <opencv2/gpu/gpu.hpp>`, keep standard OpenCV includes
- [ ] T005 [P] Clean up MarkerlessAR/SungwookAR.hpp — remove `#include <atlstr.h>`, remove `__stdcall` from thread function declarations, remove Windows-specific type usage
- [ ] T006 [P] Clean up MarkerlessAR/DBMain.cpp — remove `#include <atlstr.h>`

### GLUT/OpenGL Include Path Fixes

- [ ] T007 [P] Fix OpenGL includes in MarkerlessAR/External_Include.h — replace `#include <gl/glut.h>` with platform-conditional `#ifdef __APPLE__` using `<GLUT/glut.h>` and `<OpenGL/gl.h>`; remove `#include <opencv2/gpu/gpu.hpp>`
- [ ] T008 [P] Fix OpenGL includes in MarkerlessAR/EngineMain.cpp — replace `#include <gl/glut.h>` with same platform-conditional pattern
- [ ] T009 [P] Fix OpenGL includes in MarkerlessAR/wonjo.cpp — replace `#include "gl/glut.h"` with platform-conditional; remove `#pragma comment(lib,"glut32.lib")`
- [ ] T010 [P] Fix OpenGL includes in MarkerlessAR/HandyAR/global.h — replace `#include "GL/glut.h"` with platform-conditional
- [ ] T011 [P] Fix OpenGL includes in MarkerlessAR/calibration/BaekARCamera.h — replace `#include <glut.h>` with platform-conditional

### BRISK/Matcher MSVC Macros

- [ ] T012 [P] Fix MarkerlessAR/brisk/brisk.h — remove all 8 `#pragma comment(lib,...)` lines for OpenCV DLLs; replace `__forceinline` macro with `inline`; replace `__declspec(align(16))` with `alignas(16)`; remove `__declspec(dllexport/dllimport)` EXPORTAPI macros
- [ ] T013 [P] Fix MarkerlessAR/brisk/hammingsse.hpp — replace `__declspec(align(16))` with `alignas(16)` for SIMD data structures
- [ ] T014 [P] Fix MarkerlessAR/brisk/Matcher.h — replace `__declspec(align(16))` with `alignas(16)`

### KatoPoseEstimation DLL Macros

- [ ] T015 [P] Fix MarkerlessAR/KatoPoseEstimation/KatoPoseEstimator.h — remove `__declspec(dllexport/dllimport)` EXPORTAPI macro; replace with empty definition
- [ ] T016 [P] Fix MarkerlessAR/KatoPoseEstimation/poseestimator.h — remove `__declspec(dllexport/dllimport)` EXPORTAPI macro; replace with empty definition

### DirectX Stubs (compile-time placeholders)

- [ ] T017 Stub MarkerlessAR/d3d.cpp — replace `stdext::hash_map` with `std::unordered_map`; replace all D3D headers with forward declarations or empty types; make all functions compile as no-op stubs returning success; remove `#include <d3d9.h>`, `<d3dx9.h>`
- [ ] T018 Stub MarkerlessAR/wonjo.h — replace all `LPDIRECT3D*` types with void* or float[16] placeholders; replace `D3DXMATRIXA16` with `float[16]` typedef; replace `HWND` with void*; replace `HRESULT` with int; replace `LPCSTR` with `const char*`; replace `D3DMATERIAL9` with struct placeholder; replace `LPMESH` with void*
- [ ] T019 Stub MarkerlessAR/wonjo.cpp — make all D3D functions compile as no-ops using the stub types from wonjo.h
- [ ] T020 Stub MarkerlessAR/cam.cpp — replace `D3DXMATRIXA16` with float[16]; replace `GetDevice()->SetTransform()` with no-op

### Win32 Stubs (compile-time placeholders)

- [ ] T021 Stub MarkerlessAR/External_Include.h Win32 types — replace `#include <windows.h>` and `#include <process.h>` with `#include <thread>` and `#include <mutex>`; replace `HANDLE` thread handles with `std::thread*` (nullptr-initialized); replace `CRITICAL_SECTION` with `std::mutex`; replace `unsigned __stdcall` function signatures with `void` functions; replace `HWND` with `void*`
- [ ] T022 Stub MarkerlessAR/EngineMain.cpp — replace `WinMain` with `int main(int argc, char** argv)`; stub out `RegisterClass/CreateWindow/ShowWindow` calls; stub message loop; replace `AllocConsole/freopen` with nothing; replace `_beginthreadex` with comments (actual replacement in Sprint 3); replace `ExitProcess` with `exit(0)`; remove `#include <windows.h>` and `#include <process.h>`

- [ ] T023 Verify Sprint 1: run `mkdir build && cd build && cmake .. && make` and confirm zero compilation errors on macOS (rendering is no-op stubs)

**Checkpoint**: Project compiles on macOS with stub rendering. Sprint 1 PR ready.

---

## Phase 3: User Story 1 — Build from Source on macOS (Priority: P1)

**Goal**: Migrate all OpenCV legacy C API to modern C++ API so the project compiles with Homebrew OpenCV 4.x. After this phase, the project fully compiles with real (not stubbed) OpenCV code.

**Independent Test**: `cmake --build build` succeeds with zero errors using OpenCV 4.x from Homebrew.

**Sprint 2 PR scope**: All OpenCV migration tasks below.

### OpenCV Migration: Core Headers

- [ ] T024 [P] [US1] Migrate MarkerlessAR/External_Include.h — remove `IplImage img_binary` static, remove `CvCapture *capture_C` static, remove GPU module include, ensure `cv::VideoCapture` and `cv::Mat` types are used for all global state
- [ ] T025 [P] [US1] Migrate MarkerlessAR/EngineMain.cpp OpenCV types — replace `IplImage*` globals with `cv::Mat`, replace `CvCapture*` with `cv::VideoCapture`, replace `CvSize` with `cv::Size`, update all `cvCreateImage/cvReleaseImage` calls, update `imread/imshow/waitKey` calls if using old syntax

### OpenCV Migration: Calibration Module (High Effort)

- [ ] T026 [US1] Migrate MarkerlessAR/calibration/Camera.h — replace all `CvMat*` members (intrinsic, distortion, rotation, translation, rotation3by3, P) with `cv::Mat`
- [ ] T027 [US1] Migrate MarkerlessAR/calibration/BaekARCamera.h — replace `CvPoint3D64f/CvPoint3D32f` parameter types with `cv::Point3d/cv::Point3f`, replace `CvPoint2D32f/CvPoint2D64f` with `cv::Point2f/cv::Point2d`, replace `CvMat*` members with `cv::Mat`, update all function signatures
- [ ] T028 [US1] Migrate MarkerlessAR/calibration/BaekARCamera.cpp (Part 1: lines 1-600) — replace all `cvCreateMat()` with `cv::Mat()`, replace `cvmGet/cvmSet` with `mat.at<double>()`, replace `CV_MAT_ELEM` with `mat.at<>()`, replace `cvReleaseMat` with RAII (remove manual release), replace `cvCreateImage/cvReleaseImage` with `cv::Mat`
- [ ] T029 [US1] Migrate MarkerlessAR/calibration/BaekARCamera.cpp (Part 2: lines 601-1185) — continue same migration pattern for remaining functions including `ProjectPointToCamera` overloads, `setProjectionMatrix`, `featurePoseEstimation`, `D3DXMakeProjectionMatrix`, `D3DXMakeViewMatrix`
- [ ] T030 [US1] Migrate MarkerlessAR/calibration/cvFindExtrinsicCameraParams3.h/.cpp — replace entire function with wrapper around `cv::solvePnP()`, or migrate all internal CvMat usage to cv::Mat; update function signature to accept cv::Mat parameters
- [ ] T031 [US1] Verify calibration module compiles: build and check all calibration/*.cpp files compile without errors

### OpenCV Migration: Kato Pose Estimation (High Effort)

- [ ] T032 [US1] Migrate MarkerlessAR/KatoPoseEstimation/KatoPoseEstimator.h — replace `CvPoint2D*` and `CvPoint3D*` array pointers with `std::vector<cv::Point2f>` and `std::vector<cv::Point3d>`, replace `CvMat*` members with `cv::Mat`
- [ ] T033 [US1] Migrate MarkerlessAR/KatoPoseEstimation/KatoPoseEstimator.cpp (833 lines) — replace all CvPoint constructors, CvMat allocations, `cvmGet/cvmSet` calls; update `arGetTransMatSub` and related functions to use cv::Mat; replace `cvFindExtrinsicCameraParams3` call with cv::solvePnP if applicable
- [ ] T034 [US1] Verify KatoPoseEstimation module compiles: build and check KatoPoseEstimation/*.cpp files

### OpenCV Migration: HandyAR Module (Medium Effort)

- [ ] T035 [P] [US1] Migrate MarkerlessAR/HandyAR/MixGaussian.cpp/.h — replace cvCreateMat/CvMat with cv::Mat
- [ ] T036 [P] [US1] Migrate MarkerlessAR/HandyAR/PoseEstimation.cpp/.h — replace CvMat operations with cv::Mat
- [ ] T037 [P] [US1] Migrate MarkerlessAR/HandyAR/ColorDistribution.cpp/.h — replace cvCreateMat with cv::Mat
- [ ] T038 [P] [US1] Migrate MarkerlessAR/HandyAR/FingerTip.cpp/.h — replace CvMat/CvPoint with cv::Mat/cv::Point
- [ ] T039 [P] [US1] Migrate MarkerlessAR/HandyAR/FingertipPoseEstimation.cpp/.h — replace CvMat with cv::Mat
- [ ] T040 [P] [US1] Migrate MarkerlessAR/HandyAR/FingertipTracker.cpp/.h — replace CvMat with cv::Mat
- [ ] T041 [P] [US1] Migrate MarkerlessAR/HandyAR/HandRegion.cpp/.h — replace CvMat with cv::Mat
- [ ] T042 [US1] Verify HandyAR module compiles: build and check all HandyAR/*.cpp files

### OpenCV Migration: Feature Detection & Core (Low Effort)

- [ ] T043 [P] [US1] Migrate MarkerlessAR/BaekAR.cpp/.hpp — update any remaining legacy OpenCV types
- [ ] T044 [P] [US1] Migrate MarkerlessAR/Detector.cpp/.hpp — update any remaining legacy OpenCV types
- [ ] T045 [P] [US1] Migrate MarkerlessAR/SungwookFeature.cpp/.hpp — update any remaining legacy OpenCV types
- [ ] T046 [P] [US1] Migrate MarkerlessAR/brisk/projection.h — replace IplImage/CvMat usage with cv::Mat
- [ ] T047 [P] [US1] Check MarkerlessAR/agast/*.cc files for any CvPoint usage and migrate if present

- [ ] T048 [US1] Verify Sprint 2: full build succeeds with OpenCV 4.x — `cmake --build build` with zero errors; all ~83 source files compile and link

**Checkpoint**: Project fully compiles on macOS with Homebrew OpenCV 4.x. Sprint 2 PR ready. US1 (Build from Source) is complete.

---

## Phase 4: User Story 2 — Run AR Engine on macOS (Priority: P2)

**Goal**: Replace Win32 threading + windowing with std::thread + GLFW, replace DirectX 9 rendering with OpenGL. Application opens a window, displays camera feed, and renders AR overlays.

**Independent Test**: Launch binary → window opens → camera feed visible → point at reference image → 3D overlay appears.

### Sprint 3: Threading + GLFW Windowing (window opens)

- [ ] T049 [US2] Replace Win32 threading in MarkerlessAR/External_Include.h — change `HANDLE hMatchingThread[2]` to `std::thread* matchingThread[2]`, change `HANDLE hTrackingThread[2]` to `std::thread* trackingThread[2]`, change `HANDLE hDrawThread` to `std::thread* drawThread`, change `HANDLE hMutex` to `std::mutex`, change `CRITICAL_SECTION hCriticalSection[2]` to `std::mutex criticalSection[2]`, change thread function signatures from `unsigned __stdcall func(void*)` to `void func(int)`
- [ ] T050 [US2] Replace Win32 threading in MarkerlessAR/EngineMain.cpp — replace all `_beginthreadex()` calls with `std::thread` construction, replace `_endthreadex()` with natural function return, replace `EnterCriticalSection/LeaveCriticalSection` with `std::lock_guard<std::mutex>`, replace `CreateMutex()` with std::mutex initialization, remove `__stdcall` from ThreadBRISKMatching/ThreadTracking/ThreadDraw signatures
- [ ] T051 [US2] Replace Win32 windowing in MarkerlessAR/EngineMain.cpp — replace `WinMain()` with `int main(int argc, char** argv)`, add `glfwInit()` and `glfwCreateWindow(640, 480, "BackAR", NULL, NULL)`, add `glfwMakeContextCurrent()`, replace message loop `while(PeekMessage)` with `while(!glfwWindowShouldClose(window)) { mainLoop(); glfwPollEvents(); }`, add `glfwTerminate()` cleanup
- [ ] T052 [US2] Implement GLFW input callbacks in MarkerlessAR/EngineMain.cpp — replace `WndProc` with `glfwSetKeyCallback` (map VK_ESCAPE to GLFW_KEY_ESCAPE), replace `GetKeyState(VK_LBUTTON)` with `glfwGetMouseButton`, replace `GetCursorPos/ScreenToClient` with `glfwGetCursorPos`
- [ ] T053 [US2] Verify Sprint 3: application compiles, launches, and opens a GLFW window (blank/black is OK); threads start without crash; ESC key closes the window

**Checkpoint**: Sprint 3 PR ready. Application opens a window and starts threads.

### Sprint 4: OpenGL Rendering (camera feed + AR overlay)

#### Rendering Core

- [ ] T054 [US2] Rewrite MarkerlessAR/d3d.cpp rendering init — replace `AAR3DInitD3D(HWND)` with OpenGL setup function (context already created by GLFW); set up `glEnable(GL_DEPTH_TEST)`, `glEnable(GL_LIGHTING)`, viewport, clear color
- [ ] T055 [US2] Rewrite MarkerlessAR/d3d.cpp frame lifecycle — replace `BeginRender()` with `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`, replace `Flip()` with `glfwSwapBuffers(window)`
- [ ] T056 [US2] Rewrite MarkerlessAR/d3d.cpp camera preview — replace `AAR3DDrawCameraPreview(char*, int, int)` with OpenGL texture upload: `glGenTextures/glBindTexture/glTexImage2D` for BGR camera data, draw full-screen textured quad using `glBegin(GL_QUADS)/glEnd`
- [ ] T057 [US2] Rewrite MarkerlessAR/d3d.cpp mesh rendering — replace `D3DXLoadMeshFromX()` with simple OBJ loader or direct vertex array from bunny.h data; replace `DrawMesh()` with `glBegin/glEnd` or `glDrawArrays`; replace `DrawSubset()` with OpenGL draw calls; replace `std::unordered_map` mesh cache
- [ ] T058 [US2] Rewrite MarkerlessAR/d3d.cpp render states — replace D3D render states with OpenGL equivalents: `D3DRS_CULLMODE` → `glEnable(GL_CULL_FACE)`, `D3DRS_ZENABLE` → `glEnable(GL_DEPTH_TEST)`, `D3DRS_ALPHABLENDENABLE` → `glEnable(GL_BLEND)`, `D3DRS_LIGHTING` → `glEnable(GL_LIGHTING)`
- [ ] T059 [US2] Rewrite MarkerlessAR/d3d.cpp lighting/materials — replace `D3DLIGHT9/D3DMATERIAL9` setup with `glLightfv(GL_LIGHT0, ...)` and `glMaterialfv(GL_FRONT, ...)`

#### Matrix & Math

- [ ] T060 [US2] Rewrite MarkerlessAR/wonjo.h types — replace `D3DXMATRIXA16` typedef with `float[16]` or GLM `glm::mat4`; replace `D3DXVECTOR3` with `glm::vec3` or `float[3]`; replace all function signatures to use new types; replace `LPMESH` with custom mesh struct pointer; remove all D3D headers
- [ ] T061 [US2] Rewrite MarkerlessAR/wonjo.cpp matrix functions — replace `ChangeD3DXMatrix()` with direct float[16] copy, replace `ChangeLinearMatrix()` with float[16] extraction, replace `MultiMatrix()` with GLM mat4 multiply or manual 4x4 multiply, replace `InverseMatrix()` with GLM inverse or manual implementation using `D3DXMatrixDeterminant/D3DXMatrixInverse` → `glm::inverse`
- [ ] T062 [US2] Rewrite MarkerlessAR/wonjo.cpp SetProjectionMatrix/SetModelViewMatrix — replace D3D `SetTransform(D3DTS_PROJECTION/VIEW)` with `glMatrixMode(GL_PROJECTION/GL_MODELVIEW); glLoadMatrixf()`
- [ ] T063 [US2] Rewrite MarkerlessAR/wonjo.cpp picking — replace `CalcPickingRay/TransformRay/Picking` using `gluUnProject()` instead of D3D ray math; replace `D3DXIntersectTri` with manual ray-plane intersection
- [ ] T064 [US2] Remove/stub MarkerlessAR/wonjo.cpp screen capture code — remove `FindWindow/GetDC/PrintWindow/CreateDIBSection` GDI code (Windows-only screen capture); stub `AAR3DTexturing` to no-op or use camera frame instead

#### Camera Matrix Integration

- [ ] T065 [US2] Rewrite MarkerlessAR/cam.cpp — replace `D3DXMATRIXA16` with float[16], replace `SetProjectionMatrix` to use `glMatrixMode(GL_PROJECTION); glLoadMatrixf()`, replace `SetModelViewMatrix` to use `glMatrixMode(GL_MODELVIEW); glLoadMatrixf()` with coordinate flip (OpenGL→D3D handedness was already handled; verify correct for OpenGL)

#### Main Loop Integration

- [ ] T066 [US2] Update MarkerlessAR/EngineMain.cpp mainLoop() — replace all `wonjo_dx::` namespace calls with new OpenGL function calls; wire camera frame capture (`cv::VideoCapture::read`) → OpenGL texture upload → display; wire pose estimation matrices → `glLoadMatrixf` → 3D mesh overlay; ensure `glfwSwapBuffers` called at end of frame
- [ ] T067 [US2] Update MarkerlessAR/EngineMain.cpp InitializeEngineMain() — replace `wonjo_dx::AAR3DInitD3D(hWnd)` with OpenGL init function; wire up `cv::VideoCapture(0)` camera open; load reference image and compute initial features/descriptors
- [ ] T068 [US2] Verify Sprint 4: application opens window, displays live camera feed; when pointed at reference image, 3D object overlay appears; ESC closes cleanly

**Checkpoint**: Sprint 4 PR ready. Camera feed visible, AR overlay works. US2 (Run AR Engine) is substantially complete.

---

## Phase 5: User Story 3 — Updated README + Polish (Priority: P3)

**Goal**: Document build instructions, fix runtime edge cases, clean up Windows artifacts.

**Independent Test**: A developer unfamiliar with BackAR can build and run it by following the README alone.

**Sprint 5 PR scope**: All tasks below.

### Runtime Polish

- [ ] T069 [US3] Add camera error handling in MarkerlessAR/EngineMain.cpp — check `cv::VideoCapture::isOpened()` after open; print clear error message if camera not available (permissions or missing hardware); exit gracefully instead of crashing
- [ ] T070 [US3] Add asset path validation in MarkerlessAR/EngineMain.cpp InitializeEngineMain() — check existence of calibration/camera.dat, image/ directory, database/ directory before use; print missing file paths to stderr if not found
- [ ] T071 [US3] Fix all relative asset paths — ensure MarkerlessAR/image/, MarkerlessAR/calibration/, MarkerlessAR/database/, MarkerlessAR/3dobjects/ paths resolve correctly relative to the built executable; update CMakeLists.txt to copy or symlink asset directories to build output if needed
- [ ] T072 [P] [US3] Verify feature detection end-to-end — launch application, point camera at bundled reference image, confirm BRISK/AGAST features detected and matched within 3 seconds
- [ ] T073 [P] [US3] Verify pose estimation end-to-end — confirm Kato pose estimator produces correct 3D transformations, 3D overlay aligns with detected marker
- [ ] T074 [US3] Verify stability — run application continuously for 5+ minutes; confirm no crashes, no memory leaks (check with Instruments or leaks command)

### Documentation

- [ ] T075 [US3] Write README.md with sections: Project Description (what BackAR is), Prerequisites (macOS 13+, Xcode CLI tools, Homebrew), Install Dependencies (brew install commands), Build Instructions (cmake + make), Run Instructions (how to launch and test AR), Troubleshooting (camera permissions, missing deps), Original Project History (link to original repo, acknowledgments)

### Cleanup

- [ ] T076 [P] [US3] Remove Windows build artifacts — delete BaekAR.sdf, BaekAR.suo, BaekAR.sln, BaekAR_Sample/ directory, all .vcxproj/.vcxproj.filters/.vcxproj.user files, Engine/ output directory with .tlog and .log files
- [ ] T077 [P] [US3] Remove CNAME file (GitHub Pages domain config, not needed)
- [ ] T078 [US3] Final verification — clean clone, follow README only, install deps, build, run, confirm AR works end-to-end

**Checkpoint**: Sprint 5 PR ready. README complete, all edge cases handled, Windows artifacts removed. US3 complete.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Foundational)**: Depends on Phase 1 — BLOCKS all user stories
- **Phase 3 (US1 - Build)**: Depends on Phase 2 — can start after MSVC cleanup
- **Phase 4 (US2 - Run)**: Depends on Phase 3 — needs compilable OpenCV code first
- **Phase 5 (US3 - README)**: Depends on Phase 4 — needs working application to document

### Sprint → PR Mapping

| Sprint | Phases | PR Deliverable | Verification |
|--------|--------|----------------|-------------|
| Sprint 1 | Phase 1 + Phase 2 | Compilable skeleton (stubs) | `cmake --build build` zero errors |
| Sprint 2 | Phase 3 | Compiles with real OpenCV 4.x | `cmake --build build` zero errors with Homebrew OpenCV |
| Sprint 3 | Phase 4 (T049-T053) | Window opens, threads run | App launches, GLFW window visible |
| Sprint 4 | Phase 4 (T054-T068) | Camera feed + AR overlay | Camera displays, 3D overlay on marker |
| Sprint 5 | Phase 5 | Fully functional + documented | End-to-end AR + README verification |

### Within Each Phase

- Tasks marked [P] within the same phase can run in parallel
- Sequential tasks (no [P]) depend on prior tasks in that phase
- Verification tasks (T023, T048, T053, T068, T078) must run after all preceding tasks in their phase

### Parallel Opportunities

**Phase 2** (most parallelizable):
```
Parallel group A: T003, T004, T005, T006 (different header files)
Parallel group B: T007, T008, T009, T010, T011 (GLUT includes in different files)
Parallel group C: T012, T013, T014 (BRISK module files)
Parallel group D: T015, T016 (KatoPoseEstimation headers)
```

**Phase 3** (HandyAR parallelizable):
```
Parallel group: T035, T036, T037, T038, T039, T040, T041 (independent HandyAR files)
Parallel group: T043, T044, T045, T046, T047 (independent low-effort files)
```

**Phase 5** (cleanup parallelizable):
```
Parallel group: T072, T073 (independent verification tasks)
Parallel group: T076, T077 (independent cleanup tasks)
```

---

## Implementation Strategy

### MVP First (US1 Only — Sprint 1+2)

1. Complete Phase 1: CMake build system
2. Complete Phase 2: MSVC cleanup + stubs
3. Complete Phase 3: OpenCV migration
4. **STOP and VALIDATE**: `cmake --build build` succeeds with zero errors
5. Sprint 1 PR + Sprint 2 PR merged

### Incremental Delivery

1. Sprint 1 PR: Compilable skeleton → merge to branch
2. Sprint 2 PR: Full compilation with OpenCV 4.x → merge (US1 complete)
3. Sprint 3 PR: Window opens, threads run → merge
4. Sprint 4 PR: Camera + AR overlay working → merge (US2 complete)
5. Sprint 5 PR: Polish + README → merge (US3 complete, feature done)

Each sprint builds on the previous one. Every PR is independently verified as compilable and progressively runnable on macOS.

---

## Notes

- Total tasks: 78 (T001-T078)
- [P] tasks = different files, no dependencies within phase
- No test tasks included (manual verification per sprint)
- Commit after each task or logical group
- OpenCV migration (Phase 3) is the highest-effort phase (~30 files, 537+ API call changes)
- DirectX→OpenGL rewrite (Phase 4, Sprint 4) is the highest-risk phase
- AGAST module needs NO changes (platform-neutral code)
- SSE intrinsics should work on Apple Clang without changes (verify during Sprint 2)
