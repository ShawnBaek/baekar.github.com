# BaekAR — Development Guide

## Project

**BaekAR** is a markerless augmented-reality engine — Sungwook Baek's master's thesis (Yonsei CS, 2012). It combines three subsystems:

1. **Vision** — natural-feature tracking on a printed reference image, producing 6-DoF camera pose.
2. **Hand interaction** — adaptive skin-color hand segmentation + fingertip detection, producing a 6-DoF hand pose for pointing/clicking AR content.
3. **Window-as-3D** (the thesis novelty) — capture a real desktop application's window pixels and render them as a textured 3D plane locked to the marker, which the user can pick and interact with.

Original: Windows / VS2010 / OpenCV 2.3 / DirectX 9. This repo ports it to macOS arm64 with OpenCV 4.13 (Homebrew) + GLFW + OpenGL legacy fixed-function. Thesis PDF: `Master_Thesis_Yonsei_University_Computer_Science_Sungwook_Baek_BaekAR.pdf` at the repo root.

## Build & run (macOS)

```sh
brew install opencv glfw cmake
cmake -S . -B build && cmake --build build
open build/BaekAR.app
```

First launch triggers macOS camera permission. Reset with `tccutil reset Camera com.baekar.engine`.

## Architecture

| Subsystem | Files | Notes |
|---|---|---|
| Vision pipeline | `MarkerlessAR/EngineMain.cpp`, `SungwookFeature.cpp`, `SungwookTransformFunctions.cpp` | `cv::BRISK::create(60,2)` → BFMatcher (Hamming) → `findHomography(RANSAC)` → `cv::calcOpticalFlowPyrLK`. Two worker threads: `ThreadBRISKMatching`, `ThreadTracking`. |
| Hand interaction | `MarkerlessAR/HandyAR/` (`HandRegion`, `MixGaussian`, `FingerTip`, `FingertipTracker`, `FingertipPoseEstimation`) | Adaptive skin via Gaussian-mixture lookup → contour + distance-transform → fingertip tracking (Kalman + template match) → Zhang's calibration on 5 fingertips. Wired in `mainLoop` at `EngineMain.cpp:818,833`. |
| Rendering | `MarkerlessAR/wonjo.{h,cpp}`, `d3d.cpp`, `cam.cpp` | D3D9-style API (`AAR3DInitD3D`, `BeginRender`, `AAR3DDrawCameraPreview`, `DrawPlane`, `AAR3DDrawMesh`, `Picking`) implemented over OpenGL fixed-function on macOS. |
| Camera | `MarkerlessAR/HandyAR/Capture.cpp` | Single `cv::VideoCapture` owned by `gCapture`; frames shared to worker threads via `g_sharedFrame` + mutex. |

## Compat shim layer (`MarkerlessAR/compat/`)

| File | Purpose |
|---|---|
| `d3d_stub.h` | D3DXMATRIXA16, D3DXVECTOR3, IDirect3DDevice9_stub, D3DX math (Inverse/Determinant/Rotation/Vec3Transform). |
| `win32_stub.h` | Win32 API: HANDLE, CRITICAL_SECTION → `std::mutex`, `_beginthreadex` → `std::thread`. |
| `opencv_compat.h` | Wrappers for OpenCV C-API removed in 4.x: `cvFindHomography`, `cvRodrigues2`, `cvProjectPoints2`, `cvLoadImage`, `cvFindContours`, `cvDistTransform`, etc. |
| `cvkalman_stub.h` | `CvKalman` and `CvRandState` shimmed onto `cv::KalmanFilter` / `cv::RNG`. |
| `macos_camera_auth.{h,mm}` | AVFoundation `[AVCaptureDevice requestAccessForMediaType:]` dance. Called once before any `VideoCapture::open()`. |

## macOS specifics

- **Camera auth**: must call `RequestCameraPermission()` (in `compat/macos_camera_auth.mm`) before `cv::VideoCapture::open()`, otherwise it fails silently with "not authorized". Set `OPENCV_AVFOUNDATION_SKIP_AUTH=1` so OpenCV's broken built-in path is bypassed.
- **One AVFoundation client per camera**: `gCapture` is the only `cv::VideoCapture` that opens the device. Worker threads consume frames from `g_sharedFrame`.
- **GL thread**: `glGenTextures` and any GL call must run on the GLFW context thread (the main thread). `InitializeEngineMain` runs inline from `mainLoop` for that reason — do not move it to a worker.
- **`.app` bundle launch**: launch via `open build/BaekAR.app` (LaunchServices), not direct exec — the camera prompt only appears via LaunchServices.
- **Deprecation warnings**: `#define GL_SILENCE_DEPRECATION` before any GL include suppresses macOS's noise about legacy fixed-function GL.

## Pitfalls (don't repeat past mistakes)

- **No explicit destructor calls on stack objects.** `vec.~vector<T>()` / `mat.~Mat()` is UB on libc++ — the next iteration writes through freed storage and trips `POINTER_BEING_FREED_WAS_NOT_ALLOCATED`. Use `.clear()` / `.release()` or just let scope exit handle it. (See PR #4: `EngineMain.cpp` ThreadBRISKMatching cleanup.)
- **Empty-match guards before `findHomography` / `perspectiveTransform`.** `if (a.size()<5 && b.size()<5)` is wrong — use `||`, plus check `a.size()==b.size()` and `H.empty()` afterwards.
- **Bundled BRISK is not `cv::BRISK`.** `MarkerlessAR/brisk/` predates OpenCV's `Feature2D` interface; calling `detect()` on it throws "function not implemented" under OpenCV 4. Use `cv::BRISK::create(...)`.
- **`AAR3DTexturing` is Win32-only.** It uses `FindWindow`/`GetDC`/`PrintWindow` to capture a target window's pixels. The macOS equivalent (ScreenCaptureKit) is pending.
- **`using namespace cv;`** causes `utils::` ambiguity vs `cv::utils::` — use `::utils::`.
- **Korean code comments are intentional** — preserve them across edits.

## Sprint history

| | What | Status |
|---|---|---|
| Sprint 1 | Build system + MSVC cleanup → compiles on macOS arm64 | Done |
| Sprint 3 | GLFW window + `std::thread` / `std::mutex` real threading | Done |
| Sprint 4 | DirectX → OpenGL rendering via compat shim | Done |
| PR #2 | Camera permission + main-thread init, app runs end-to-end | Done |
| PR #3 | Project rename: BackAR → BaekAR | Done |
| PR #4 | Re-enable AR matching/tracking workers (UB + guard fixes) | Done |
| Pending | HandyAR end-to-end runtime verification on macOS | In progress |
| Pending | macOS window-as-AR-texture (ScreenCaptureKit) | Planned |
| Pending | Per-object pick + interaction (`Contents` scene model) | Planned |
| Pending | `.X` mesh loading on macOS (currently RGB-axes placeholder) | Deferred |
| Pending | Metal renderer backend | Deferred |

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
