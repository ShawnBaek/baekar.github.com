# BaekAR architecture

BaekAR is being changed around the 2012 engine, not rewritten. The first goal is to make the existing camera, tracking, pose, rendering, and hand pipelines separable while keeping their results the same.

## Dependency direction

```text
apps -> application -> core
           ^
           |
    adapters and platform

legacy code is reached through adapters
```

The target folders are:

```text
apps/baekar/             executable and composition root
src/core/                geometry, scene, and tracking value types
src/application/         use cases, lifecycle, and ports
src/adapters/            OpenCV, Kato, HandyAR, and OpenGL adapters
src/platform/macos/      AVFoundation and ScreenCaptureKit
legacy/                  reviewed 2012 and platform-specific code
assets/                  runtime images, calibration, and meshes
tests/characterization/  current behavior
tests/integration/       adapter and application boundaries
```

This is the destination, not the structure for the first PR. Files move only after a boundary builds and has characterization coverage.

## Rules

1. Do not change an algorithm while extracting its interface.
2. Move files with `git mv`, separately from formatting or code changes.
3. Keep OpenGL work on the thread that owns the GLFW context.
4. Keep macOS frameworks outside `core` and `application`.
5. Keep the current OpenCV types at adapter seams until removing them does not add frame copies.
6. Check source and asset provenance before calling a folder `legacy` or `third_party`.
7. Compare every refactoring PR with the merged macOS baseline.

## Current seams

| Seam | Current implementation | First boundary |
|---|---|---|
| Frame source | `HandyAR/Capture` | camera, video, AVFoundation, synthetic, and dummy strategies |
| Marker tracking | `compat/MultiMarkerDetector` | reusable tracking library, then `IMarkerTracker` |
| Pose | `KatoPoseEstimation` and calibration | `IPoseEstimator` adapter |
| Hand interaction | `HandyAR` | `IHandTracker` adapter |
| Scene | `Contents` | application-owned scene model |
| Rendering | `wonjo`, `d3d`, OpenGL compatibility | renderer adapter |
| macOS | `compat/macos_*` | platform adapters |

`EngineMain.cpp` remains the composition root until these boundaries exist. It becomes smaller in later PRs; the first PR does not split it.

## Runtime invariants

- Camera, video, synthetic marker, and window capture modes keep the same command-line behavior.
- Marker identities in one published result belong to the same frame sequence.
- Tracking can lose markers during occlusion and recover them after they return.
- Rendering and event polling remain on the GLFW/OpenGL context thread.
- Camera permission and application bundle behavior remain macOS responsibilities.
- The original BRISK, AGAST, Kato, and HandyAR calculations stay available for comparison.

## Planned order

1. Architecture baseline and characterization tests.
2. C++17 boundary for new code, with legacy code isolated when needed.
3. Application facade and explicit lifecycle ownership.
4. Frame-source strategies.
5. Tracking and pose ports.
6. Hand interaction and rendering adapters.
7. Folder moves with no formatting changes.

The yearly markerless AR work starts after this foundation. A yearly experiment should enter through a tracking, pose, mapping, or reconstruction interface instead of being added directly to `EngineMain.cpp`.
