# Research: macOS Port of BackAR

**Branch**: `001-macos-port` | **Date**: 2026-02-08

## R1: DirectX 9 Replacement Strategy

**Decision**: Replace DirectX 9 with legacy OpenGL (compatibility profile) + GLFW windowing.

**Rationale**:
- macOS supports OpenGL up to 4.1 (legacy profile available for GL 2.1 compatibility)
- The original code uses fixed-function pipeline features (immediate mode, matrix stack, lighting) which map directly to OpenGL 1.x/2.x
- DirectX functions have near-1:1 OpenGL equivalents: `SetTransform()` → `glLoadMatrixf()`, `BeginScene/EndScene` → `glClear/SwapBuffers`, vertex buffers → `glVertexPointer`
- D3DX math (D3DXMATRIXA16, D3DXMatrixScaling, etc.) → manual 4x4 float arrays or GLM library
- Mesh loading (D3DXLoadMeshFromX for .X format) → custom OBJ loader or use existing bunny.h header data
- GLFW chosen over SDL2 because it's lighter weight, OpenGL-focused, and available via Homebrew

**Alternatives considered**:
- Metal: Too complex for a legacy port, requires complete rewrite
- SDL2 + OpenGL: Heavier than needed; GLFW is sufficient
- Vulkan/MoltenVK: Overkill for fixed-function pipeline code

**Scope of D3D code**:
- `d3d.cpp` (913 lines): Device init, mesh loading, texture management, rendering
- `wonjo.cpp` (542 lines): Matrix conversions, picking, window capture
- `wonjo.h` (129 lines): D3D type declarations and function prototypes
- `cam.cpp` (54 lines): Matrix setup for camera
- `EngineMain.cpp` (1951 lines): Uses D3D via wonjo_dx namespace throughout mainLoop()

## R2: Win32 API Replacement Strategy

**Decision**: Replace Win32 windowing with GLFW, Win32 threading with C++11 std::thread/std::mutex.

**Rationale**:
- GLFW provides window creation, input handling, and OpenGL context in ~10 lines
- C++11 threading is standard and supported by Apple Clang
- The Win32 message loop maps to GLFW's event polling loop
- Mouse/keyboard input maps to GLFW callbacks

**Mapping**:
| Win32 | Replacement |
|-------|-------------|
| `WinMain()` | `main()` |
| `RegisterClass/CreateWindow` | `glfwCreateWindow()` |
| `PeekMessage/DispatchMessage` | `glfwPollEvents()` |
| `WndProc` | GLFW key/mouse callbacks |
| `_beginthreadex` | `std::thread` |
| `CRITICAL_SECTION` | `std::mutex` |
| `CreateMutex/HANDLE` | `std::mutex` |
| `AllocConsole` | Not needed (stdout works on macOS) |
| `ExitProcess` | `exit()` |

## R3: OpenCV Migration Strategy

**Decision**: Migrate from OpenCV 2.3.1 legacy C API to OpenCV 4.x modern C++ API.

**Rationale**:
- OpenCV 4.x removed the legacy C API entirely (IplImage, CvMat, cv* functions)
- Modern API (cv::Mat) is more memory-safe and has equivalent functionality
- Homebrew provides OpenCV 4.x; version 2.3.1 is not available

**Scale of migration**:
- IplImage: 124 occurrences across 30+ files
- CvMat: 251 occurrences (heaviest in calibration/ and KatoPoseEstimation/)
- CvCapture: 3 occurrences
- CvPoint/CvPoint2D/CvPoint3D: 158 occurrences
- cvCreateMat/cvReleaseMat: 150 occurrences
- cvCreateImage/cvReleaseImage: 71 occurrences

**Key migration patterns**:
| Legacy C API | Modern C++ API |
|-------------|----------------|
| `IplImage*` | `cv::Mat` |
| `CvMat*` / `cvCreateMat(r,c,type)` | `cv::Mat(r,c,type)` |
| `cvReleaseMat(&mat)` | Automatic (RAII) |
| `CvCapture*` | `cv::VideoCapture` |
| `CvPoint2D32f` | `cv::Point2f` |
| `CvPoint3D64f` | `cv::Point3d` |
| `CV_MAT_ELEM(*mat,type,r,c)` | `mat.at<type>(r,c)` |
| `cvmSet(mat,r,c,val)` | `mat.at<double>(r,c) = val` |
| `cvmGet(mat,r,c)` | `mat.at<double>(r,c)` |
| `cvFindExtrinsicCameraParams3()` | `cv::solvePnP()` |

**Critical files** (by migration effort):
1. `calibration/BaekARCamera.cpp` (1185 lines, 70+ CvMat allocations)
2. `KatoPoseEstimation/KatoPoseEstimator.cpp` (833 lines, CvPoint/CvMat heavy)
3. `calibration/cvFindExtrinsicCameraParams3.cpp` (246 lines, pure CvMat)
4. `HandyAR/*.cpp` (multiple files with CvMat usage)

## R4: MSVC-Specific Code Replacement

**Decision**: Replace with standard C++ and platform-conditional macros where needed.

**Mapping**:
| MSVC Code | Replacement |
|-----------|-------------|
| `stdext::hash_map` | `std::unordered_map` |
| `__forceinline` | `inline` |
| `__declspec(align(16))` | `alignas(16)` |
| `__declspec(dllexport/import)` | Remove (single binary, not DLL) |
| `__stdcall` | Remove (not needed) |
| `#include <atlstr.h>` | Remove (CString not used) |
| `#pragma comment(lib,...)` | Remove (CMake handles linking) |
| `#pragma warning(disable:...)` | Remove or use `-Wno-*` in CMake |

## R5: OpenGL/GLUT Include Paths

**Decision**: Use macOS framework paths with conditional includes.

**Mapping**:
```cpp
#ifdef __APPLE__
  #include <GLUT/glut.h>
  #include <OpenGL/gl.h>
#else
  #include <GL/glut.h>
  #include <GL/gl.h>
#endif
```

Found in 6 files: External_Include.h, EngineMain.cpp, wonjo.cpp, global.h, HandyAR/global.h, calibration/BaekARCamera.h

## R6: GPU Module

**Decision**: Remove GPU module includes entirely.

**Rationale**: `cv::gpu::` headers are included in 2 files but zero GPU functions are called. Safe to remove.

## R7: Build System

**Decision**: CMake 3.20+ with Homebrew dependency resolution.

**Rationale**: CMake is the de facto standard for C++ cross-platform builds, well-supported by Apple Clang, and integrates with `find_package()` for OpenCV and GLFW.

## R8: Mesh Loading (.X Format)

**Decision**: Replace D3DXLoadMeshFromX with a simple OBJ loader or use the embedded bunny.h vertex data directly.

**Rationale**: The .X mesh format is Microsoft-proprietary. The project already has bunny.h with vertex data embedded as a C header. For the iPad model, an .obj file exists alongside the .X file. A minimal OBJ loader can replace the D3DX mesh loading.
