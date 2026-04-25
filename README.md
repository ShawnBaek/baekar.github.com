BaekAR
======

Fully Open Natural Feature Tracking Augmented Reality Engine + Interaction
Engine (6DoF Hand Pose Estimation).

The engine was originally written for Visual Studio 2010, OpenCV 2.3.1, and
Direct3D 9. This branch ports the natural-feature tracking pipeline to
**macOS** with **Metal** rendering and a **modern OpenCV** (4.x) vision
stack. The 6DoF hand-pose module is staged for a follow-up port; see
`MarkerlessAR/LEGACY.md` for the migration map.

Build & run on macOS
--------------------

Prerequisites:

    brew install cmake opencv glfw

Optional — fetch Apple's metal-cpp headers (only needed if you extend the
renderer in plain C++; the bundled Objective-C++ renderer does not require
them):

    ./third_party/fetch_metal_cpp.sh

Configure and build:

    cmake -S . -B build -G Ninja
    cmake --build build

Run (grant Camera permission on first launch):

    ./build/baekar

Optional flags:

    --reference PATH     # marker image (default MarkerlessAR/image/yejin.jpg)
    --calibration PATH   # intrinsics file (default calibration/calibration.txt)
    --offline IMG        # render a single still frame and exit (CI smoke)

Press `ESC` to quit. When the reference image is visible to the camera you
will see a 3-axis (R/G/B) overlay locked to the marker plane.

Repository layout (after port)
------------------------------

    engine/
      main.mm                       # macOS host: GLFW + capture + worker threads
      vision/
        BriskTracker.{h,cpp}        # cv::BRISK matching + LK tracking
        FeaturePose.{h,cpp}         # solvePnP + GL/Metal projection / view
      render_metal/
        Renderer.h                  # cross-platform renderer interface
        MetalRenderer.mm            # CAMetalLayer + Metal pipelines
        Shaders.metal               # background quad + axis line shaders
    tests/test_feature_pose.cpp     # smoke test for FeaturePose
    third_party/metal-cpp/          # Apple metal-cpp headers (fetched)
    MarkerlessAR/                   # original sources (mostly retired; see LEGACY.md)

Status
------

| Module                              | macOS status |
|-------------------------------------|--------------|
| Natural-feature AR (BRISK + pose)   | Running      |
| Metal renderer (background + axis)  | Running      |
| HandyAR (6DoF fingertip pose)       | Deferred     |
| Marker-based pose (Kato)            | Deferred     |
| `.X` mesh loading                   | Deferred     |
