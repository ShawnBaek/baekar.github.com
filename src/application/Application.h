#ifndef BAEKAR_APPLICATION_APPLICATION_H
#define BAEKAR_APPLICATION_APPLICATION_H

#include "application/AppConfig.h"
#include "application/ports/IEnginePipeline.h"
#include "application/ports/IFrameSource.h"
#include "application/ports/IMarkerTracker.h"
#include "application/ports/IPoseEstimator.h"
#include "application/ports/IWindow.h"

#include <string>
#include <vector>

namespace baekar {

// Everything the application needs, chosen by the composition root.
struct Dependencies {
    IWindow& window;
    IFrameSource& source;  // already open; closed on shutdown
    IMarkerTracker& tracker;
    IPoseEstimator& poseEstimator;
    IEnginePipeline& pipeline;
};

// Facade over the whole run: prepare, initialize, frame loop, shutdown.
// Replaces the InitializeEngineMain / mainLoop / ReleaseEngineMain trio.
class Application {
public:
    Application(AppConfig config, std::vector<std::string> markerImages,
                std::string calibrationPath, Dependencies dependencies);

    // Returns the process exit code.
    int run();

    long renderedFrames() const { return renderedFrames_; }
    long framesWithMarker() const { return framesWithMarker_; }

private:
    bool startUp();
    TrackingResult track();
    bool saveScreenshot();
    void shutDown();

    AppConfig config_;
    std::vector<std::string> markerImages_;
    std::string calibrationPath_;
    Dependencies deps_;
    Frame frame_;
    long renderedFrames_ = 0;
    long framesWithMarker_ = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_APPLICATION_H
