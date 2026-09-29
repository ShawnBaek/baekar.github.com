#ifndef BAEKAR_LEGACY_ENGINE_H
#define BAEKAR_LEGACY_ENGINE_H

// Entry points into the 2012 engine in EngineMain.cpp. Plain C++ so that the
// C++17 application layer can call them without including legacy headers.

#include <opencv2/core.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace legacy_engine {

struct Options {
    bool handTracking = true;
    bool cameraInput = true;     // false for synthetic or replayed frames
    bool windowCapture = false;  // macOS ScreenCaptureKit window-as-texture
};

// One frame from the application's IFrameSource (640x480 BGR).
struct FrameInput {
    cv::Mat bgr;
    std::uint64_t sequence = 0;
    std::int64_t tickCount = 0;
    bool live = true;  // false for the "camera unavailable" placeholder
};

// What the application's IMarkerTracker and IPoseEstimator produced for
// this frame. Matrices use the 2012 D3DX memory layout.
struct TrackedMarker {
    std::size_t index = 0;
    std::string name;
    bool found = false;
    bool tracking = false;
    std::array<cv::Point2f, 4> outline{};  // detection corners
    int inliers = 0;
    int trackedPoints = 0;
};

struct TrackingInput {
    std::vector<TrackedMarker> markers;
    bool drivesPose = false;  // 2012 single-marker pipeline: pose + 3D overlay
    std::array<float, 16> projection{};
    bool poseValid = false;
    std::array<float, 16> view{};
};

struct Pointer {
    double x = 0.0;
    double y = 0.0;
    bool leftDown = false;
};

// Stores options and opens the optional macOS window capture. Marker
// selection, camera permission and the camera picker live in the
// application now.
bool Prepare(const Options& options, int argc, char** argv);
// GLUT, the D3D-over-GL stub and the 2012 GL state. Needs a current context.
bool InitializeRenderer();
// Former InitializeEngineMain(): HandyAR initialization.
bool Start(const FrameInput& firstFrame);
// Former mainLoop() body: hand tracking and rendering for one frame.
void RenderFrame(const FrameInput& frame, const TrackingInput& tracking, const Pointer& pointer);
void Shutdown();

}  // namespace legacy_engine

#endif  // BAEKAR_LEGACY_ENGINE_H
