# Legacy / unbuilt sources

These directories and files are kept in source control for diff history and
as reference for the macOS port, but are **not compiled** by the new
`CMakeLists.txt`. They target Visual Studio 2010 + OpenCV 2.3.1 + Direct3D 9
+ Win32 and do not build outside that environment.

| Path                                              | Replacement                                                       |
|---------------------------------------------------|-------------------------------------------------------------------|
| `agast/`                                          | `cv::AgastFeatureDetector` inside OpenCV's BRISK pipeline         |
| `brisk/`                                          | `cv::BRISK::create()` + `cv::BFMatcher(cv::NORM_HAMMING)`         |
| `calibration/cvFindExtrinsicCameraParams3.{h,cpp}`| `cv::solvePnP` (see `engine/vision/FeaturePose.cpp`)              |
| `calibration/Camera.{h,cpp}`                      | `baekar::CameraIntrinsics` + helpers in `engine/vision/FeaturePose.h` |
| `d3d.cpp`, `wonjo.{h,cpp}`, `cam.cpp`             | `engine/render_metal/MetalRenderer.mm`                            |
| `EngineMain.cpp`                                  | `engine/main.mm`                                                  |
| `HandyAR/`                                        | Deferred — see "Out of scope" in the port PR                      |
| `KatoPoseEstimation/`                             | Deferred — was used only for legacy fiducial markers              |
| `Sungwook*.{cpp,hpp}`                             | Folded into `engine/vision/BriskTracker.cpp` (`niceHomography` etc) |
| `BaekAR.{cpp,hpp}`, `BaekAR_Sample/baekar.cpp`    | Empty / never built                                                |
| `*.sln`, `*.vcxproj*`, `*.suo`, `*.sdf`           | Replaced by `CMakeLists.txt`                                       |

## Restoring HandyAR (6DoF fingertip pose)

The HandyAR module compiles against the pre-2.4 OpenCV C API
(`IplImage`, `CvKalman`, `CvRandState`, `cvFindContours`, `CvVideoWriter`)
and an OpenGL texture upload path. Porting it requires:

1. Replace `IplImage*` with `cv::Mat` in `HandyAR/*.{h,cpp}`.
2. Replace `CvKalman` with `cv::KalmanFilter` (init + step API differs).
3. Replace `CvRandState` / `cvRand*` with `cv::RNG`.
4. Replace `CvVideoWriter` with `cv::VideoWriter`.
5. Move texture uploads from OpenGL into the Metal renderer
   (extend `Renderer` with a `drawTexturedQuad` or a hand mesh primitive).

Once ported, re-enable by adding the files to `baekar_engine` in
`CMakeLists.txt` and switching the `BAEKAR_ENABLE_HANDYAR` flag in
`engine/main.mm`.
