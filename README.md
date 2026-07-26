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
brew install cmake opencv glfw glm freeglut assimp

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

open build/BaekAR.app
```

BaekAR needs camera permission on first launch.

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

| Year | Focus |
|---|---|
| 2013 | Semi-dense visual odometry |
| 2014 | Direct SLAM and keyframe maps |
| 2015 | ORB-SLAM and relocalization |
| 2016 | Monocular, stereo, and RGB-D SLAM |
| 2017 | Visual-inertial tracking and world anchors |
| 2018 | Learned local features |
| 2019 | Scene understanding and occlusion |
| 2020 | Learned hand tracking |
| 2021 | Visual-inertial and multi-map SLAM |
| 2022 | Neural implicit maps |
| 2023 | 3D Gaussian Splatting |
| 2024 | Learned dense reconstruction |
| 2025 | 3D visual foundation models |
| 2026 | Streaming spatial maps |

Every yearly PR will include research notes, one working experiment, comparison with the previous version, and macOS verification.

## Master's thesis

[Master Thesis — Sungwook Baek — BaekAR](Master_Thesis_Yonsei_University_Computer_Science_Sungwook_Baek_BaekAR.pdf)

## License

License and asset review will be part of the foundation work. Third-party source and assets will keep their original attribution and license information.
