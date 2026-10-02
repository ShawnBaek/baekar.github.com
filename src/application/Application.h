#ifndef BAEKAR_APPLICATION_APPLICATION_H
#define BAEKAR_APPLICATION_APPLICATION_H

#include "application/AppConfig.h"
#include "application/InteractionController.h"
#include "application/ports/IFrameSource.h"
#include "application/ports/IHandTracker.h"
#include "application/ports/IMarkerTracker.h"
#include "application/ports/IPoseEstimator.h"
#include "application/ports/IRenderer.h"
#include "application/ports/IScene.h"
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
    IHandTracker& handTracker;
    IRenderer& renderer;
    IScene& scene;
    IWindowTextureSource* windowTexture = nullptr;  // optional, already open
};

struct RunSettings {
    std::vector<std::string> markerImages;
    std::string calibrationPath;
    long maxFrames = 0;          // 0: run until the window closes
    std::string screenshotPath;  // written after the last frame
};

// Facade over the whole run: start-up, frame loop, shutdown. Replaces the
// InitializeEngineMain / mainLoop / ReleaseEngineMain trio of the 2012
// EngineMain.cpp.
class Application {
public:
    Application(RunSettings settings, Dependencies dependencies);

    // Returns the process exit code.
    int run();

    long renderedFrames() const { return renderedFrames_; }
    long framesWithMarker() const { return framesWithMarker_; }
    long framesWithHandPose() const { return framesWithHandPose_; }  // gesture held
    long framesWithHand() const { return framesWithHand_; }          // hand visible

private:
    bool startUp();
    TrackingResult track();
    void renderFrame(const TrackingResult& tracking, const HandState& hand);
    bool saveScreenshot();
    void shutDown();

    RunSettings settings_;
    Dependencies deps_;
    InteractionController interaction_;
    Frame frame_;
    cv::Mat windowPixels_;
    long renderedFrames_ = 0;
    long framesWithMarker_ = 0;
    long framesWithHandPose_ = 0;
    long framesWithHand_ = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_APPLICATION_H
