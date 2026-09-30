# BaekAR — Development Guide

## Project

**BaekAR** is a markerless augmented-reality engine — Sungwook Baek's master's thesis (Yonsei CS, 2012). It combines three subsystems:

1. **Vision** — natural-feature tracking on a printed reference image, producing 6-DoF camera pose.
2. **Hand interaction** — adaptive skin-color hand segmentation + fingertip detection, producing a 6-DoF hand pose for pointing/clicking AR content.
3. **Window-as-3D** (the thesis novelty) — capture a real desktop application's window pixels and render them as a textured 3D plane locked to the marker, which the user can pick and interact with.

Original: Windows / VS2010 / OpenCV 2.3 / DirectX 9. This repo ports it to macOS arm64 with OpenCV 4.13 (Homebrew) + GLFW + OpenGL legacy fixed-function. Thesis PDF: `Master_Thesis_Yonsei_University_Computer_Science_Sungwook_Baek_BaekAR.pdf` at the repo root.

## Build & run

```sh
# macOS (Homebrew's `opencv` is OpenCV 5; the 2012 code needs opencv@4)
brew install cmake opencv@4 glfw glm freeglut assimp
# Linux
sudo apt-get install cmake libopencv-dev libglfw3-dev freeglut3-dev libglm-dev libassimp-dev

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel
ctest --test-dir build
open build/BaekAR.app                                   # macOS camera
./build/BaekAR --simulate-marker yejin.jpg --frames 300 --screenshot out.png   # headless check
```

Proof videos for roadmap milestones: `scripts/record_proof.sh build <milestone>` writes `docs/progress/<milestone>/` (MP4, README GIF, results). Add an entry to the README progress log for each milestone.

First launch triggers macOS camera permission. Reset with `tccutil reset Camera com.baekar.engine`. Run `BaekAR --help` for all flags (`--camera`, `--marker`, `--replay`, `--record`, `--tracker`, `--no-hand`, `--interactive`, `--window-capture`).

## Architecture

Ports and adapters; see `docs/architecture/README.md` and `docs/architecture/design.md`.

| Layer | Path | Notes |
|---|---|---|
| Composition root | `apps/baekar/main.cpp` | The only place that names concrete adapters. |
| Core | `src/core/` | `Frame`, `MarkerObservation`, `Pose`, `Mat4` (2012 D3DX layout), `HandState`. |
| Application | `src/application/` | `Application` facade (frame loop), `AppConfig`, `InteractionController`, ports in `ports/`. C++17, `-Werror`. |
| Adapters | `src/adapters/` | Frame sources, trackers, pose, hand, renderer, scene, GLFW window. |
| macOS | `src/platform/macos/` | AVFoundation camera (Continuity Camera), camera menu + permission, ScreenCaptureKit. Only target that links Apple frameworks. |
| 2012 engine | `legacy/MarkerlessAR/` | C++14 `baekar_legacy`. Reached only through `legacy/bridge/` wrappers. |
| Data | `assets/` | Symlinked next to the binary at build time. |

The 2012 vision pipeline (`legacy/bridge/legacy_marker_tracker.cpp`): `cv::BRISK::create(30,3)` → 2-NN Hamming, ratio 0.65 → `findHomography(RHO, 2.5)` (RANSAC fallback, ≥8 inliers) → NCC template tracking in a ±20 px window. HandyAR (`legacy/MarkerlessAR/HandyAR/`): Gaussian-mixture skin → contour + distance transform → fingertips (Kalman + template) → pose from 5 fingertips.

## Compat shim layer (`legacy/MarkerlessAR/compat/`)

| File | Purpose |
|---|---|
| `d3d_stub.h` | D3DXMATRIXA16, D3DXVECTOR3, IDirect3DDevice9_stub, D3DX math. `GetTransform`/`GetViewport` do nothing; do not rely on them. |
| `win32_stub.h` | Win32 types, CRITICAL_SECTION → `std::mutex`, `_beginthreadex` → `std::thread`. |
| `opencv_compat.h` | OpenCV C-API functions removed in 4.x. |
| `cvkalman_stub.h` | `CvKalman` / `CvRandState` on `cv::KalmanFilter` / `cv::RNG`. |

## macOS specifics

- **Camera auth**: `AvFoundationFrameSource::open()` requests permission before opening the device.
- **One camera client**: the `IFrameSource` owned by the application is the only reader; trackers get frames through `submit()`.
- **GL thread**: GL calls (renderer, HandyAR start, window texture upload) run on the GLFW context thread. Trackers never touch GL.
- **`.app` bundle launch**: launch via `open build/BaekAR.app` (LaunchServices), not direct exec — the camera prompt only appears via LaunchServices.
- **Deprecation warnings**: `#define GL_SILENCE_DEPRECATION` before any GL include suppresses macOS's noise about legacy fixed-function GL.

## Pitfalls (don't repeat past mistakes)

- **No explicit destructor calls on stack objects.** `vec.~vector<T>()` / `mat.~Mat()` is UB on libc++. Use `.clear()` / `.release()` or let scope exit handle it.
- **Empty-match guards before `findHomography` / `perspectiveTransform`.** `if (a.size()<5 && b.size()<5)` is wrong — use `||`, plus check `a.size()==b.size()` and `H.empty()` afterwards.
- **Bundled BRISK is not `cv::BRISK`.** `MarkerlessAR/brisk/` predates OpenCV's `Feature2D` interface; calling `detect()` on it throws "function not implemented" under OpenCV 4. Use `cv::BRISK::create(...)`.
- **`AAR3DTexturing` is Win32-only.** The macOS equivalent is `ScreenCaptureWindowSource` (`--window-capture`).
- **Case-sensitive includes.** macOS file systems hide include-case mismatches; Linux CI does not.
- **Keep the Linux build working.** `__APPLE__` guards only Apple-framework code; portable GL/POSIX code uses `#ifndef _WIN32`.
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
| PRs #5–#30 | Continuity Camera, camera menu, ScreenCaptureKit window texture, Assimp `.X` meshes, matching tuning | Done |
| PRs #31–#34 | Merge to master, multi-marker simulation, synchronized tracking, architecture baseline | Done |
| Foundation 2–7 | C++17 boundary + CI, application facade, frame sources, tracking/pose/hand/render/scene ports, folder layout | Done |
| Pending | HandyAR detection reliability on current cameras | Open |
| Pending | Metal renderer as a second `IRenderer` | Planned |
| Pending | OpenCV 5 (remove the C API from the 2012 code) | Planned |

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
