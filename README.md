# BaekAR

This was my old project. I made it as my Master's thesis research at Yonsei University in 2012.

BaekAR is a markerless AR engine with natural feature tracking, 6DoF camera pose estimation, 3D rendering, and hand interaction.

My last markerless AR research was 2012.

Now I want to refactor it with modern C++, verify it working well on macOS, and catch up markerless AR trends per year.

This is my original research project. I want to preserve its originality and Git history while continuing it on macOS.

## Current state

The original project was built with Visual Studio 2010, OpenCV 2.3.1, DirectX 9, BRISK, and AGAST.

The macOS version is now merged into `master`. The original 2012 `master` is saved in [`snapshot/2012-original`](https://github.com/ShawnBaek/baekar.github.com/tree/snapshot/2012-original).

Every new PR will merge into `master` without rewriting the original Git history.

## Build on macOS

```bash
brew install cmake opencv@4 glfw glm freeglut assimp

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

open build/BaekAR.app
```

BaekAR needs camera permission on first launch.

Homebrew's `opencv` formula is OpenCV 5, which removed the C API that the 2012 engine still uses. Install `opencv@4`; CMake finds it automatically.

## Build on Linux

Linux builds the same engine for headless verification and CI. macOS-only features (Continuity Camera, camera menu, window capture) are not built there.

```bash
sudo apt-get install cmake libopencv-dev libglfw3-dev freeglut3-dev libglm-dev libassimp-dev

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build
```

To run it with a virtual camera looking at one of the marker images:

```bash
open -n build/BaekAR.app --args --simulate-marker yejin.jpg
```

The virtual camera moves, changes depth and direction, tilts, and rotates. It uses the same marker detection, tracking, pose, and rendering pipeline as the real camera.

To simulate multiple markers:

```bash
open -n build/BaekAR.app --args \
  --simulate-marker yejin.jpg \
  --simulate-marker fish.jpg \
  --simulate-marker cola.jpg
```

Each marker moves independently. BRISK finds them from one shared frame, and batched optical flow tracks them on the same frame timeline. The simulator was verified with ten markers.

Use distinct marker images. Visually similar images are rejected because their identity is ambiguous.

## Plan

### Step 1 — Foundation and refactoring

Continue from the merged macOS version. Verify the current behavior first, and then refactor it with smaller PRs.

- Folder structure
- Modern C++ and RAII
- Camera, tracking, pose, hand tracking, and rendering separation
- C++ design patterns where they are useful
- Tests and recorded fixtures
- Native macOS rendering and app packaging

The original 2012 pipeline will stay available for comparison.

Architecture decisions and the folder migration are tracked in [`docs/architecture`](docs/architecture/README.md).

### Step 2 — Verify on macOS

Verify real and simulated camera input, marker detection and tracking, 3D object overlay, hand and fingertip tracking, ScreenCaptureKit, permissions, relaunch, and Debug and Release builds.

### Step 3 — Catch up markerless AR trends

Every year will have one focused PR.

#### Shared groundwork (before 2013)

These are built once and reused by every yearly PR.

- **iPhone sensor capture.** Continuity Camera sends only video to the Mac. It does not send depth, LiDAR, IMU, camera intrinsics, or ARKit poses. A small iPhone capture app records these streams into a BaekAR dataset folder. The engine replays that folder through its frame-source port.
- **Reference hardware.** iPhone 17 Pro:
  - 48 MP Fusion (main), Ultra Wide, and Telephoto cameras
  - LiDAR scanner and front TrueDepth camera
  - Accelerometer, gyroscope, magnetometer, and barometer
  - ARKit world tracking, scene depth, and scene reconstruction
- **Learned-model runtime: Core ML.** Models are converted with `coremltools` into one `.mlpackage`. The same package runs on the Mac and on the iPhone. Core ML chooses the Neural Engine, GPU, or CPU. The engine reaches it through a port adapter in `src/platform/macos`. On Linux, the same ports use ONNX Runtime on the CPU, or the feature is off.
- **Common evaluation.**
  - Metrics: trajectory error (ATE and RPE), reprojection error, and frame time.
  - Data: BaekAR's own iPhone recordings and one public dataset (TUM RGB-D or EuRoC).
  - ARKit's pose is recorded as a reference trajectory.

| Year | Focus | Representative work | iPhone sensors used |
|---|---|---|---|
| 2013 | Semi-dense visual odometry | Semi-dense VO (Engel et al.) | Main camera, intrinsics |
| 2014 | Direct SLAM and keyframe maps | LSD-SLAM, SVO | Main camera |
| 2015 | Feature SLAM and relocalization | ORB-SLAM | Main camera |
| 2016 | Stereo and RGB-D SLAM | ORB-SLAM2, DSO | LiDAR depth, TrueDepth; Main + Ultra Wide captured together as a stereo pair |
| 2017 | Visual-inertial tracking and world anchors | VINS-Mono, ARKit/ARCore | Gyroscope and accelerometer, synchronized with frames; ARKit pose as reference |
| 2018 | Learned local features | SuperPoint | Main camera; Core ML on the Neural Engine |
| 2019 | Scene understanding and occlusion | Depth- and segmentation-based occlusion | LiDAR depth, person segmentation |
| 2020 | Learned matching and hand tracking | SuperGlue; learned 3D hand pose (replaces HandyAR's skin model) | Main and TrueDepth cameras; Vision hand pose as a baseline |
| 2021 | Detector-free matching and learned SLAM | LoFTR, DROID-SLAM | Main camera, IMU |
| 2022 | Neural implicit maps | iMAP, NICE-SLAM | LiDAR RGB-D |
| 2023 | 3D Gaussian Splatting | 3DGS; LightGlue for faster matching | RGB with ARKit poses, LiDAR depth for initialization |
| 2024 | Learned dense reconstruction | DUSt3R, MASt3R, Gaussian Splatting SLAM | Main, Ultra Wide, and Telephoto cameras (multi-view) |
| 2025 | 3D visual foundation models | VGGT, MASt3R-SLAM | Main camera; LiDAR depth for evaluation |
| 2026 | Streaming spatial reconstruction | Recurrent/streaming 3D reconstruction (e.g. CUT3R) | Full sensor stream: RGB, LiDAR, IMU |

Changes from the first draft:

- 2021 no longer repeats ORB-SLAM (2016) and visual-inertial tracking (2017). It moves to learned matching and learned SLAM.
- Learned matching (SuperGlue, LoFTR, LightGlue) is added. It is the direct replacement for BaekAR's BRISK + RANSAC matching.
- Learned hand tracking stays in 2020. It can be pulled earlier because HandyAR's skin-color detection is the weakest part today.

Every yearly PR will include:

- Research notes
- One working experiment behind an engine port, not added directly to `EngineMain.cpp`
- Comparison with the previous version on the common evaluation set
- macOS verification

## Master's thesis

[Master Thesis — Sungwook Baek — BaekAR](Master_Thesis_Yonsei_University_Computer_Science_Sungwook_Baek_BaekAR.pdf)

## License

License and asset review will be part of the foundation work. Third-party source and assets will keep their original attribution and license information.
