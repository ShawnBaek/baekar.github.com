# BackAR

**A Markerless Augmented Reality Engine with 6DoF Hand Pose Estimation**

BackAR is a real-time markerless augmented reality system that combines BRISK/AGAST feature-based object recognition with fingertip-based 6DoF hand pose estimation. It was originally developed as **Sungwook Baek's Master's thesis project** circa 2012, built for Windows with Visual Studio 2010, DirectX 9, and OpenCV 2.3.1.

**After 13 years**, the project is being reborn — ported to modern macOS (Apple Silicon) and co-developed with [Claude Code](https://claude.ai/claude-code) by Anthropic.

---

## What It Does

BackAR detects a reference image in a live camera feed, computes the camera's 6DoF pose relative to that image, and overlays 3D objects in real-time. It also features a hand tracking mode that estimates camera pose from fingertip positions — no markers required.

**Two Tracking Modes:**
1. **Feature-based markerless tracking** — Detects and tracks a reference image using BRISK descriptors and AGAST corners, then estimates pose via homography decomposition
2. **Fingertip-based hand pose tracking** — Segments the hand via skin color modeling, detects fingertips using distance transform + curvature analysis, and estimates 6DoF pose from fingertip correspondences

---

## Architecture

```
Camera Feed (640x480)
       |
       v
+------------------+     +-------------------+
| Feature Detection |     | Hand Segmentation |
| (AGAST corners)  |     | (Skin color GMM)  |
+--------+---------+     +---------+---------+
         |                          |
         v                          v
+------------------+     +-------------------+
| Feature Matching |     | Fingertip Detection|
| (BRISK binary    |     | (Distance transform|
|  descriptors,    |     |  + curvature +     |
|  Hamming dist)   |     |  K-means cluster)  |
+--------+---------+     +---------+---------+
         |                          |
         v                          v
+------------------+     +-------------------+
| Homography       |     | Fingertip Tracking |
| (RANSAC)         |     | (Kalman filters)   |
+--------+---------+     +---------+---------+
         |                          |
         +------------+-------------+
                      |
                      v
            +-------------------+
            | Pose Estimation   |
            | (Kato-Billinghurst|
            |  / PnP solver)    |
            +--------+----------+
                     |
                     v
            +-------------------+
            | 3D Rendering      |
            | (OpenGL)          |
            +-------------------+
```

---

## Modules

### BRISK Feature Descriptor (`brisk/`)
Binary Robust Invariant Scalable Keypoints. Generates 512-bit binary descriptors that are scale- and rotation-invariant, enabling fast Hamming distance matching. Based on the paper by Leutenegger, Chli & Siegwart (ICCV 2011, ETH Zurich).

### AGAST Corner Detector (`agast/`)
Adaptive and Generic Accelerated Segment Test. Faster than FAST corner detection with multiple pattern variants (5_8, 7_12s, 7_12d, 9_16). Provides the keypoint locations that BRISK then describes. By Elmar Mair (2010).

### Kato Pose Estimation (`KatoPoseEstimation/`)
Implements the Kato & Billinghurst transformation matrix computation. Takes 2D-3D point correspondences from detected marker corners and computes the camera-to-marker transformation matrix (rotation + translation) with iterative refinement.

### HandyAR Hand Tracking (`HandyAR/`)
Complete hand tracking pipeline:
- **HandRegion** — Skin color segmentation using Mixture of Gaussians in YCrCb color space with 16x16x16 histogram bins
- **FingerTip** — Fingertip detection via distance transform, contour curvature analysis (angle threshold 0.5 rad), and template matching with K-means clustering
- **FingertipTracker** — Kalman filter bank (up to 20 simultaneous trackers) for temporal smoothing
- **PoseEstimation** — Computes extrinsic camera parameters from fingertip 3D-2D correspondences; outputs rotation (matrix + quaternion) and translation

### Camera Calibration (`calibration/`)
Camera intrinsic/extrinsic parameter management, distortion correction, and a custom `cvFindExtrinsicCameraParams3` implementation for pose estimation from known 3D-2D correspondences.

### Multi-Scale Multi-Rotation Database
For robust recognition under arbitrary viewpoints, the system pre-computes feature databases across:
- **3 scale levels**: 0.5x, 1.0x, 2.0x
- **20 rotation steps**: 0 to 340 degrees (18-degree increments)
- **60 total templates** per reference image

### Custom Feature Utilities (`SungwookFeature.cpp`)
Image-set matching pipeline with multi-scale/rotation descriptor extraction, homography validation, and NCC patch tracking for temporal coherence.

---

## Project Structure

```
MarkerlessAR/
  EngineMain.cpp          # Application entry point, main loop, threading
  BaekAR.cpp              # AR engine controller (detector, tracker, pose, contents)
  d3d.cpp                 # Rendering layer (OpenGL on macOS, DirectX 9 on Windows)
  wonjo.cpp/.h            # 3D math, matrix operations, mesh management
  cam.cpp                 # Camera matrix setup for rendering pipeline
  SungwookFeature.cpp     # Feature extraction and matching utilities
  SungwookUtility.cpp     # Image transformation and helper functions
  Detector.cpp            # Object detection coordinator
  Contents.cpp            # 3D content management
  brisk/                  # BRISK feature descriptor (ETH Zurich)
  agast/                  # AGAST corner detector
  calibration/            # Camera calibration and pose computation
  KatoPoseEstimation/     # Kato-Billinghurst pose estimator
  HandyAR/                # Hand tracking + fingertip 6DoF pose estimation
  compat/                 # Cross-platform compatibility layers
    d3d_stub.h            #   DirectX 9 type stubs
    win32_stub.h          #   Win32 API type stubs
    opencv_compat.h       #   OpenCV 2->4 removed C API wrappers
    cvkalman_stub.h       #   CvKalman/CvRandState stubs
  image/                  # Reference images for detection
  3dobjects/              # 3D mesh assets (bunny, hand, iPad models)
  database/               # Pre-computed feature databases (YAML)
```

---

## Build Instructions

### Prerequisites

- macOS 13+ (Apple Silicon or Intel)
- Xcode Command Line Tools
- Homebrew

### Install Dependencies

```bash
brew install opencv glfw glm freeglut cmake
```

### Build

```bash
cd BackAR
mkdir build && cd build
cmake ..
cmake --build .
```

This produces `build/BackAR.app` (a macOS application bundle).

### Run

```bash
open build/BackAR.app
```

On first launch, macOS will ask for **camera permission** — click "Allow" to grant access. BackAR requires a camera to function.

If the camera feed does not appear, verify that camera access is granted in **System Settings > Privacy & Security > Camera**.

### Controls

- **ESC** — Quit the application
- **Left mouse click** — Place/interact with 3D content (picking)

---

## Current Status

| Sprint | Status | Description |
|--------|--------|-------------|
| Sprint 1 | Done | Build system (CMake) + MSVC cleanup. Compiles on macOS ARM64 with 0 errors. |
| Sprint 2 | Skipped | OpenCV C API already works via compat wrappers. No refactoring needed. |
| Sprint 3 | Done | GLFW windowing + C++11 std::thread/std::mutex replacing Win32 API |
| Sprint 4 | Done | DirectX 9 rendering replaced with OpenGL. Camera feed displays in GLFW window. |
| Sprint 5 | Planned | Runtime polish, end-to-end verification |

---

## History

This project was originally developed as **Sungwook Baek's Master's thesis** at **Yonsei University, Department of Computer Science** (circa 2012) — a markerless augmented reality engine combining state-of-the-art feature detection (BRISK/AGAST), pose estimation (Kato-Billinghurst), and a novel hand tracking system (HandyAR) for 6DoF fingertip-based camera pose estimation.

The original thesis is included in this repository: [`Master_Thesis_Yonsei_University_Computer_Science_Sungwook_Baek_BaekAR.pdf`](Master_Thesis_Yonsei_University_Computer_Science_Sungwook_Baek_BaekAR.pdf)

The original system was built for **Windows with Visual Studio 2010**, using DirectX 9 for rendering, OpenCV 2.3.1 for computer vision, and Win32 API for windowing and threading. It supported real-time AR at 640x480 with multi-threaded feature matching and tracking.

**13 years later**, the project is being brought back to life — ported to modern macOS (Apple Silicon) with OpenCV 4.x, OpenGL, GLFW, and C++14. This resurrection is a collaboration between the original author and **Claude Code** (Anthropic's AI coding assistant), tackling the substantial challenge of modernizing ~15,000 lines of decade-old C++ across 83 source files.

---

## Acknowledgments

- **BRISK**: Stefan Leutenegger, Margarita Chli, Roland Siegwart — ETH Zurich, Autonomous Systems Lab (ICCV 2011)
- **AGAST**: Elmar Mair (2010)
- **Kato Pose Estimation**: Hirokazu Kato, Mark Billinghurst — ARToolKit
- **macOS Port**: Co-developed with [Claude Code](https://claude.ai/claude-code) by Anthropic

---

## License

Original research code. BRISK and AGAST modules are licensed under their respective open-source licenses (see source file headers).
