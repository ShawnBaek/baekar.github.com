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

The folders (in place since stage 7):

```text
apps/baekar/             executable and composition root
src/core/                geometry, scene, and tracking value types
src/application/         use cases, lifecycle, and ports
src/adapters/            frame sources, trackers, pose, hand, renderer, scene, window
src/platform/macos/      AVFoundation, ScreenCaptureKit, Vision, camera menu and permission
legacy/MarkerlessAR/     the 2012 engine (C++14), layout unchanged
legacy/bridge/           C++14 wrappers that expose 2012 classes through plain headers
assets/                  runtime images, calibration, meshes and skin models
tests/unit/              parser and interaction state machine
tests/characterization/  2012 tracker and pose behavior
tests/integration/       frame source record/replay
```

The patterns behind this layout are in [design.md](design.md). Provenance and licenses of the 2012 code are in [legacy/README.md](../../legacy/README.md).

## Rules

1. Do not change an algorithm while extracting its interface.
2. Move files with `git mv`, separately from formatting or code changes.
3. Keep OpenGL work on the thread that owns the GLFW context.
4. Keep macOS frameworks outside `core` and `application`.
5. Keep the current OpenCV types at adapter seams until removing them does not add frame copies.
6. Check source and asset provenance before calling a folder `legacy` or `third_party`.
7. Compare every refactoring PR with the merged macOS baseline.

## Seams

| Port | Implementations |
|---|---|
| `IFrameSource` | `AvFoundationFrameSource` (macOS), `OpenCvCameraFrameSource`, `SyntheticFrameSource`, `ImageSequenceFrameSource` (replay), `DummyFrameSource`, `RecordingFrameSource` (decorator) |
| `IMarkerTracker` | `LegacySingleMarkerTracker` (2012 BRISK + NCC threads), `MultiMarkerTracker` |
| `IPoseEstimator` | `LegacyCameraPoseEstimator` (2012 `CCamera`) |
| `IHandTracker` | `LandmarkHandTracker` (pinch gesture over `IHandLandmarkDetector`), `HandyArHandTracker` (2012 baseline), `DisabledHandTracker` |
| `IHandLandmarkDetector` | `VisionHandLandmarkDetector` (macOS, Apple Vision) |
| `IRenderer` | `LegacyGlRenderer` (2012 `wonjo_dx` over OpenGL) |
| `IScene` / `IWindowTextureSource` | `ContentsScene`; `ScreenCaptureWindowSource` (macOS) |
| `IWindow` | `GlfwWindow` |

`EngineMain.cpp` is gone: its code lives in the `legacy/bridge` wrappers, and `apps/baekar/main.cpp` is the composition root.

## Runtime invariants

- Camera, synthetic marker, and window capture modes keep working. The stdin pickers now run only with `--interactive`; `--camera`, `--marker` and `--window-capture` replace them.
- Marker identities in one published result belong to the same frame sequence.
- Tracking can lose markers during occlusion and recover them after they return.
- Rendering and event polling remain on the GLFW/OpenGL context thread.
- Camera permission and application bundle behavior remain macOS responsibilities.
- The original BRISK, AGAST, Kato, and HandyAR calculations stay available for comparison.

## Foundation order

| Stage | Status |
|---|---|
| 1. Architecture baseline and characterization tests | Done (PR #34) |
| 2. C++17 boundary; legacy isolated as `baekar_legacy` (C++14); macOS + Linux CI | Done |
| 3. Application facade, `AppConfig`, worker lifecycle (stop + join) | Done |
| 4. Frame-source strategies, record and replay | Done |
| 5. Tracking and pose ports | Done |
| 6. Hand, renderer and scene ports; `InteractionController` | Done |
| 7. Folder moves with no formatting changes | Done |

Found while extracting, and fixed in the stage that touched them:

- Worker threads were detached `while(true)` loops and read their argument from a dead stack variable.
- The two 2012 tracking threads shared ~40 globals without synchronization.
- The tracking search window could leave the image and throw inside the thread, ending the process.
- Picking read the projection, view and viewport from the D3D stub device, which never fills them outside Windows.
- The projection matrix was rebuilt and appended to `projectionlog.txt` every frame.

Resolved in roadmap PR F1:

- The bundled GPL-3.0 BRISK/AGAST and `SungwookFeature.cpp` are no longer built; the project is Apache-2.0 (`LICENSE`, `THIRD_PARTY.md`).
- `CCamera::D3DXMakeViewMatrix` no longer appends to `viewlog.txt` every frame.
- OpenCV 5: the 2012 C-API code stays on OpenCV 4 and is replaced rather than ported ([ADR 0003](adr/0003-opencv-5.md)).

The yearly markerless AR work starts after this foundation. A yearly experiment enters through a port (frame source, tracker, pose, hand, renderer) and is chosen in the composition root.
