# Third-party components

BaekAR is licensed under Apache-2.0 (`LICENSE`). Every dependency, bundled source, dataset and model is listed here with its license. The policy is in [docs/roadmap.md](docs/roadmap.md#license-policy): only permissive licenses go into what BaekAR builds and ships.

## Linked by the BaekAR build

| Component | Use | License |
|---|---|---|
| OpenCV 4 | Images, features (`cv::BRISK`), geometry, video I/O | Apache-2.0 |
| GLFW 3 | Window and GL context | Zlib |
| freeglut / GLUT | Bitmap fonts, GL helpers | MIT (freeglut); Apple GLUT framework on macOS |
| OpenGL | Legacy renderer | System API |
| GLM | Header-only math | MIT |
| Assimp (optional) | Loads the `.X` meshes | BSD-3-Clause |
| Apple frameworks | AVFoundation, ScreenCaptureKit, Cocoa (macOS only) | System API |

## Bundled in `legacy/` and not built

| Path | License | Why it is not built |
|---|---|---|
| `legacy/MarkerlessAR/brisk/` | GPL-3.0-or-later | Replaced by OpenCV's `cv::BRISK` |
| `legacy/MarkerlessAR/agast/` | GPL-3.0-or-later | Only used by the bundled BRISK |
| `legacy/MarkerlessAR/SungwookFeature.cpp` | Carries BRISK's GPL-3.0 header | Its three helpers were rewritten in `legacy/bridge/marker_geometry.cpp` (identical results, checked on all 20 marker images) |

## Bundled in `legacy/` and built, with unclear terms

These remain in the 2012 comparison pipeline. They must not be shipped in an app until their authors confirm the terms, or until the yearly work replaces them.

| Path | Origin | Terms |
|---|---|---|
| `legacy/MarkerlessAR/HandyAR/` | Taehee Lee (UCLA), adapted | Unknown. Replaced by the learned hand tracker (roadmap PR H). |
| `legacy/MarkerlessAR/calibration/cvFindExtrinsicCameraParams3.*` | Taehee Lee | Unknown |
| `legacy/MarkerlessAR/calibration/BaekARCamera.*` | Wonwoo Lee (GIST) | Header says non-commercial. Not built. |
| `legacy/MarkerlessAR/KatoPoseEstimation/` | Kato & Billinghurst method, BaekAR implementation | Unknown |
| `legacy/MarkerlessAR/3dobjects/bunny/` | Stanford bunny | Stanford 3D Scanning Repository terms |

## Assets

| Path | Notes |
|---|---|
| `assets/image/` | Marker photos from the 2012 project. Some show people; confirm consent before redistributing outside this repository. |
| `assets/meshes/`, `assets/handyar/`, `assets/calibration/` | Produced by the 2012 BaekAR project |
