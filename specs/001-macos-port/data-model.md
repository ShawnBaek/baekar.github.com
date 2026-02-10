# Data Model: macOS Port of BackAR

**Branch**: `001-macos-port` | **Date**: 2026-02-08

## Overview

This is a C++ desktop application port, not a data-driven service. The "data model" describes the key code modules and their relationships, plus the file assets the application depends on.

## Module Dependency Graph

```
EngineMain (entry point)
├── wonjo_dx namespace (rendering) → REPLACE with OpenGL
│   ├── d3d.cpp (D3D device, textures, meshes)
│   ├── wonjo.cpp (matrix math, picking, screen capture)
│   └── cam.cpp (camera matrix setup)
├── BaekAR (core AR engine)
│   ├── Detector (feature detection)
│   │   ├── brisk/ (BRISK descriptor)
│   │   └── agast/ (AGAST detector)
│   ├── SungwookFeature (custom feature transforms)
│   └── SungwookAR (AR utilities, structures)
├── calibration/
│   ├── Camera/BaekARCamera (camera parameters, projection)
│   ├── cvFindExtrinsicCameraParams3 (custom pose solver)
│   └── KatoPoseEstimation/ (Kato pose estimator)
├── HandyAR/ (hand tracking)
│   ├── FingertipPoseEstimation
│   ├── FingertipTracker
│   ├── HandRegion
│   ├── MixGaussian (skin color model)
│   └── ColorDistribution
├── Contents (content management)
└── External_Include.h (global state, threading handles)
```

## File Assets

| Asset | Path | Format | Used By |
|-------|------|--------|---------|
| Reference images | `MarkerlessAR/image/*.jpg` | JPEG | Feature matching database |
| Camera calibration | `MarkerlessAR/calibration/camera.dat` | Binary | CCamera initialization |
| Fingertip data | `MarkerlessAR/calibration/fingertip_*.dat` | Binary | FingertipPoseEstimation |
| Skin distribution | `MarkerlessAR/skin.dis` | Binary | HandRegion color model |
| Non-skin distribution | `MarkerlessAR/nonskin.dis` | Binary | HandRegion color model |
| Feature database | `MarkerlessAR/database/*.yml` | YAML | Descriptor matching |
| Bunny mesh | `MarkerlessAR/3dobjects/bunny/inc/bunny.h` | C header (vertex data) | 3D overlay rendering |
| iPad model | `MarkerlessAR/3dobjects/ipad_high/iPad.obj` | OBJ | 3D overlay rendering |
| ARTag config | `MarkerlessAR/calibration/artag_1023.cf` | Config | Marker configuration |

## Threading Model

```
Main Thread (UI + Rendering)
├── mainLoop() → polls events, renders frame
│
├── ThreadBRISKMatching[0] (background)
│   └── Captures frame → detects keypoints → matches descriptors → computes homography
│
├── ThreadTracking[0] (background)
│   └── Template matching → ROI tracking → perspective transform
│
└── ThreadDraw (background, optional)
    └── Debug visualization of detection/tracking results
```

**Synchronization**: CRITICAL_SECTION[2] guards shared data between threads → will become std::mutex[2]
