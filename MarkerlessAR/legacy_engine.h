#ifndef BAEKAR_LEGACY_ENGINE_H
#define BAEKAR_LEGACY_ENGINE_H

// Entry points into the 2012 engine in EngineMain.cpp. Plain C++ so that the
// C++17 application layer can call them without including legacy headers.

#include <opencv2/core.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace legacy_engine {

struct Options {
    int cameraIndex = -1;                       // -1: platform default
    std::string markerImage;                    // empty: image/yejin.jpg
    std::vector<std::string> simulatedMarkers;  // resolved paths
    bool multiMarkerTracker = false;            // MultiMarkerDetector instead of 2012 threads
    bool handTracking = true;
    bool interactive = false;
    bool windowCapture = false;
};

// One frame from the application's IFrameSource (640x480 BGR).
struct FrameInput {
    cv::Mat bgr;
    std::uint64_t sequence = 0;
    std::int64_t tickCount = 0;
    bool live = true;  // false for the "camera unavailable" placeholder
};

struct Pointer {
    double x = 0.0;
    double y = 0.0;
    bool leftDown = false;
};

// Stores options, resolves marker paths and, with interactive, runs the
// marker picker. Camera permission and the camera picker are part of the
// macOS frame source now.
bool Prepare(const Options& options, int argc, char** argv);
// GLUT, the D3D-over-GL stub and the 2012 GL state. Needs a current context.
bool InitializeRenderer();
// Former InitializeEngineMain(): HandyAR, BRISK database, workers.
bool Start(const FrameInput& firstFrame);
// Former mainLoop() body: one frame of tracking, pose and rendering.
void RenderFrame(const FrameInput& frame, const Pointer& pointer);
// Stops and joins the worker threads.
void Shutdown();

}  // namespace legacy_engine

#endif  // BAEKAR_LEGACY_ENGINE_H
