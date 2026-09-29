# BaekAR architecture and design patterns

This document designs the structure that the remaining foundation stages (2–7 in
[README.md](README.md)) build. It is written against the code as it stands after
PR #34, and it names the exact 2012 code each piece wraps.

## 1. What the code looks like today

`MarkerlessAR/EngineMain.cpp` (2,772 lines) is the whole application:

| Concern | Where it lives today | Problem |
|---|---|---|
| Configuration | stdin pickers in `main()` (`PickCameraIndex`, `PickMarkerImage`, `WCPicker_PickWindowID`), globals `Filename[]`, `g_markerSimulation*` | Needs a person at a terminal; cannot be automated |
| Lifecycle | `InitializeEngineMain()` run lazily inside `mainLoop()`, `ReleaseEngineMain()` | Worker threads are `while(true)` loops, detached by the `CloseHandle` stub, never joined |
| Frame input | `HandyAR/Capture` (`gCapture`) + `g_sharedFrame` + `g_dummyIpl` | One class owns camera, file, synthetic and dummy input; returns `IplImage*` |
| Marker tracking | `ThreadBRISKMatching` / `ThreadTracking` (single marker) and `MultiMarkerDetector` (multi-marker simulation) | ~40 globals shared without locks; thread arguments point at a dead stack variable (`&val1`); tracking thread spins without sleeping |
| Pose | `CCamera camera` (`featurePoseEstimation`, `D3DXMake*Matrix`), `CKatoPoseEstimator` | Pose input is written into `camera.featuresResult` by the tracking threads; projection writes `projectionlog.txt` every frame |
| Hand interaction | `FingertipPoseEstimation gFingertipPoseEstimation` | Its debug image is also the camera background |
| Picking / drag | Mouse block and fingertip block in `mainLoop()` | Two copies of the same interaction logic |
| Rendering | `wonjo_dx::*` (D3D9-shaped API over OpenGL) | Portable GL code is gated by `__APPLE__`, so non-Apple builds lose the overlay |
| Platform | `compat/macos_*.mm` | Fine, but reached directly from `EngineMain.cpp` |

## 2. Architecture pattern: ports and adapters

```text
          apps/baekar  (composition root: parse config, build objects, run)
                │
                ▼
      src/application  ── owns the frame loop and use cases
         │   ports: IFrameSource, IMarkerTracker, IPoseEstimator,
         │          IHandTracker, IRenderer, IScene
         ▼
        src/core      ── value types only (Frame, MarkerObservation, Pose,
                         CameraIntrinsics, HandState, Mat4)
                ▲
                │ implement ports
      src/adapters ── wrap 2012 code and OpenCV
      src/platform/macos ── AVFoundation, ScreenCaptureKit, menus, permissions
                │
             legacy/  ── the 2012 engine, compiled unchanged as C++14
```

Dependency rules (checked by the CMake target graph, not by convention):

1. `baekar_core` links only OpenCV core. It has no GL, GLFW, Apple frameworks or legacy headers.
2. `baekar_application` links `baekar_core`. It sees ports, never adapters.
3. `baekar_adapters` links `baekar_application` and `baekar_legacy`.
4. `baekar_platform_macos` is built only on Apple and is the only target that links Apple frameworks.
5. `apps/baekar` is the only place that names concrete adapter types.

OpenCV value types (`cv::Mat`, `cv::Point2f`, `cv::Matx`) are allowed in `core` because
removing them would add frame copies (architecture rule 5).

## 3. Core types (`src/core`)

```cpp
struct Frame {                 // one camera image
    cv::Mat bgr;               // CV_8UC3, 640x480 in the current pipeline
    std::uint64_t sequence;    // strictly increasing per source
    double timestampSeconds;
};

struct MarkerObservation {     // one marker in one frame
    std::size_t markerIndex;
    std::string name;
    bool found;
    enum class Mode { Detecting, Tracking } mode;
    std::array<cv::Point2f, 4> poseCorners;     // corners the pose uses
    std::array<cv::Point2f, 4> detectionCorners;// last BRISK homography corners (overlay)
    int inliers, trackedPoints;
    std::uint64_t frameSequence;
};

struct CameraIntrinsics { double fx, fy, cx, cy; cv::Vec4d distortion; cv::Size imageSize; };

struct Mat4 { std::array<float, 16> m; };      // column-major, OpenGL convention

struct Pose { bool valid; Mat4 projection; Mat4 view; };

struct HandState { bool validPose; cv::Point2f indexFingertip; Pose pose; };
```

## 4. Ports (`src/application/ports`)

```cpp
class IFrameSource {           // Strategy
public:
    virtual ~IFrameSource() = default;
    virtual bool open() = 0;
    virtual bool read(Frame& out) = 0;      // false = no frame this tick
    virtual void close() = 0;
    virtual std::string describe() const = 0;
};

class IMarkerTracker {         // Strategy; runs its own workers
public:
    virtual ~IMarkerTracker() = default;
    virtual bool start(const std::vector<std::string>& markerImages) = 0;
    virtual void submit(const Frame& frame) = 0;               // non-blocking
    virtual std::vector<MarkerObservation> latest() const = 0; // snapshot
    virtual void stop() = 0;                                   // joins workers
};

class IPoseEstimator {         // Strategy
public:
    virtual ~IPoseEstimator() = default;
    virtual Mat4 projection() const = 0;
    virtual Pose estimate(const MarkerObservation& marker) = 0;
};

class IHandTracker {           // Strategy + Null Object
public:
    virtual ~IHandTracker() = default;
    virtual bool start(const Frame& first) = 0;
    virtual HandState process(const Frame& frame) = 0;
    virtual const cv::Mat* debugView() const = 0;   // the 2012 background image
};

class IRenderer {              // Adapter target
public:
    virtual ~IRenderer() = default;
    virtual void beginFrame() = 0;
    virtual void drawCameraBackground(cv::Mat& bgr) = 0;
    virtual void drawMarkerOverlay(const MarkerObservation&, std::size_t colorIndex, bool labelled) = 0;
    virtual void drawScreenSpaceAnchor(const MarkerObservation&) = 0; // 2012 spinning axis
    virtual void drawWorldAnchor(const Pose&) = 0;                   // DrawPlane + Arrow3Axis.X
    virtual void drawHand(const Pose&) = 0;
    virtual void endFrame() = 0;
};

class IScene {                 // pick/drag target
public:
    virtual ~IScene() = default;
    virtual int pick(int windowX, int windowY) = 0;
    virtual void select(int index) = 0;
    virtual void deselectAll() = 0;
    virtual void drag(double dx, double dy) = 0;
    virtual void render() = 0;
};
```

## 5. Design patterns and where each one is used

| Pattern | Where | Why |
|---|---|---|
| Ports and adapters (hexagonal) | whole layout | Lets the 2012 algorithms stay untouched behind interfaces while new code grows around them |
| Composition root / constructor injection | `apps/baekar/main.cpp` → `Application(Dependencies)` | One place decides which concrete classes run; tests inject fakes |
| Facade | `Application` | Replaces `InitializeEngineMain` / `mainLoop` / `ReleaseEngineMain` with `start()`, `tick()`, `shutdown()` |
| Strategy | `IFrameSource`, `IMarkerTracker`, `IPoseEstimator`, `IHandTracker` | Chosen by CLI (`--source`, `--tracker`) instead of `#ifdef` and globals |
| Adapter | `LegacySingleMarkerTracker`, `LegacyCameraPoseEstimator`, `HandyArHandTracker`, `LegacyGlRenderer`, `ContentsScene` | Wrap 2012 classes without editing their algorithms |
| Decorator | `RecordingFrameSource` | `--record DIR` saves every frame of any source without the source knowing |
| Simple factory | `makeFrameSource(config)`, `makeMarkerTracker(config)` | Maps config values to strategies in one place |
| Null Object | `DisabledHandTracker`, `DummyFrameSource` | Removes `if (g_cameraAvailable)` / `bDrawHand` branches from the loop |
| State | `InteractionController` (`Idle` → `Dragging`) | One implementation of pick-then-drag, fed by mouse or fingertip |
| Snapshot publication (latest-value) | tracker `latest()` under a mutex | Render thread never reads half-written corner arrays (the old globals did) |
| RAII | `WorkerThread` (stop token + join in destructor), `GlfwWindow`, capture handles | Threads stop and join on shutdown; no manual `CloseHandle`/`delete` |
| Builder-style config | `AppConfig` + `parseCommandLine` | All startup choices become explicit, validated, testable values |

Patterns considered and not used: Observer/event bus (only one consumer per result, a
snapshot is simpler), Singleton (the old code's globals are the problem being removed),
ECS for the scene (one or two items; `Contents` is enough).

## 6. Runtime model

```text
main thread (owns GLFW + GL)                 worker threads (owned by tracker)
─────────────────────────────                ──────────────────────────────────
source.read(frame)
recorder decorator writes frame (optional)
tracker.submit(frame) ─────────────────────▶ legacy: matching + NCC tracking loops
hand.process(frame)                          multi: synchronized BRISK + LK loop
obs = tracker.latest()   ◀──────────────────  publish snapshot (mutex)
pose = poseEstimator.estimate(obs[0])
renderer: background, overlays, anchors, hand, scene
interaction.update(mouse | fingertip)
glfwSwapBuffers / glfwPollEvents
```

* All GL calls stay on the main thread (runtime invariant).
* Workers consume frames by sequence number and sleep when there is nothing new,
  instead of reprocessing the same shared frame.
* `Application::shutdown()` stops the tracker (joins workers) before the source closes.

## 7. Configuration (`AppConfig`)

| Flag | Replaces | Default |
|---|---|---|
| `--source camera\|synthetic\|replay\|dummy` | implicit choice in `InitializeEngineMain` | `camera` (`synthetic` when `--simulate-marker` is given) |
| `--camera N` | stdin camera picker | 0 |
| `--marker PATH` | stdin marker picker, `Filename[0]` | `image/yejin.jpg` |
| `--simulate-marker PATH` (repeatable) | unchanged | – |
| `--replay DIR` / `--record DIR` | – | – |
| `--tracker legacy\|multi` | `g_multiMarkerSimulationEnabled` | `legacy` for one marker, `multi` for several |
| `--no-hand` | `_DRAW_HAND` compile flag | hand tracking on |
| `--frames N` | – | run until ESC |
| `--screenshot PATH` | – | – |
| `--interactive` | – | pickers run only with this flag and a TTY |
| `--window-capture` (macOS) | stdin window picker | off |

## 8. Target folders (stage 7)

```text
apps/baekar/               main.cpp (composition root)
src/core/                  value types
src/application/           ports, Application, AppConfig, InteractionController
src/adapters/              frame_source/, tracking/, pose/, hand/, render/, scene/
src/platform/macos/        AVFoundation, ScreenCaptureKit, camera menu, permission
legacy/MarkerlessAR/       2012 sources (moved with git mv, content unchanged)
assets/                    image/, calibration data, database/, 3dobjects/, meshes, skin models
tests/                     characterization/, unit/, integration/
```

## 9. Testing strategy

| Level | What | Source of truth |
|---|---|---|
| Unit | `AppConfig` parsing, `InteractionController` states, `Mat4` helpers | Hand-written expectations |
| Characterization | 1/3/10-marker tracking (existing), legacy single-marker tracker on synthetic frames, legacy pose estimator output stability | Current behavior |
| Integration | record → replay round trip; headless app run (`--frames`, `--screenshot`) under Xvfb in CI | Frame counts and tracked-marker logs |

## 10. Stage mapping

| Stage | Delivers from this design |
|---|---|
| 2 | Target split (`baekar_legacy` C++14, new targets C++17), portable GL build, CI |
| 3 | `AppConfig`, `Application` facade, `WorkerThread`, headless run flags |
| 4 | `IFrameSource` + camera / synthetic / dummy / replay strategies, recording decorator |
| 5 | `IMarkerTracker` (legacy + multi), `IPoseEstimator` (legacy CCamera) |
| 6 | `IHandTracker`, `IRenderer`, `IScene`, `InteractionController` |
| 7 | Folder moves only |
