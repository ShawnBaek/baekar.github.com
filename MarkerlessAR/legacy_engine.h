#ifndef BAEKAR_LEGACY_ENGINE_H
#define BAEKAR_LEGACY_ENGINE_H

// Entry points into the 2012 engine in EngineMain.cpp. Plain C++ so that the
// C++17 application layer can call them without including legacy headers.

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

struct Pointer {
    double x = 0.0;
    double y = 0.0;
    bool leftDown = false;
};

// Stores options, resolves marker paths and runs platform setup
// (camera permission and, with interactive, the stdin pickers).
bool Prepare(const Options& options, int argc, char** argv);
// GLUT, the D3D-over-GL stub and the 2012 GL state. Needs a current context.
bool InitializeRenderer();
// Former InitializeEngineMain(): capture, HandyAR, BRISK database, workers.
bool Start();
// Former mainLoop() body: one frame of capture, tracking, pose, rendering.
void RenderFrame(const Pointer& pointer);
// Stops and joins the worker threads, releases capture.
void Shutdown();

}  // namespace legacy_engine

#endif  // BAEKAR_LEGACY_ENGINE_H
