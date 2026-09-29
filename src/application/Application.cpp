#include "application/Application.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <utility>

namespace baekar {

Application::Application(AppConfig config, std::vector<std::string> markerImages,
                         std::string calibrationPath, Dependencies dependencies)
    : config_(std::move(config)),
      markerImages_(std::move(markerImages)),
      calibrationPath_(std::move(calibrationPath)),
      deps_(dependencies) {}

bool Application::startUp() {
    if (!deps_.pipeline.prepare(config_)) {
        std::fprintf(stderr, "BaekAR: platform preparation failed.\n");
        return false;
    }
    if (!deps_.pipeline.initializeRenderer()) {
        std::fprintf(stderr, "BaekAR: renderer initialization failed.\n");
        return false;
    }
    std::fprintf(stderr, "BaekAR: frame source: %s\n", deps_.source.describe().c_str());
    if (!deps_.source.read(frame_)) {
        std::fprintf(stderr, "BaekAR: the frame source produced no first frame.\n");
        return false;
    }
    if (!deps_.poseEstimator.loadCalibration(calibrationPath_))
        std::fprintf(stderr, "BaekAR: could not read calibration %s\n", calibrationPath_.c_str());
    if (!deps_.pipeline.start(frame_)) {
        std::fprintf(stderr, "BaekAR: engine start failed.\n");
        return false;
    }
    std::fprintf(stderr, "BaekAR: marker tracker: %s\n", deps_.tracker.describe().c_str());
    for (const std::string& marker : markerImages_) std::fprintf(stderr, "  marker %s\n", marker.c_str());
    if (!deps_.tracker.start(markerImages_, frame_)) {
        std::fprintf(stderr, "BaekAR: marker tracker start failed.\n");
        if (markerImages_.size() > 1)
            std::fprintf(stderr, "BaekAR: use visually distinct marker images.\n");
        return false;
    }
    return true;
}

TrackingResult Application::track() {
    deps_.tracker.submit(frame_);
    TrackingResult result;
    result.markers = deps_.tracker.latest();
    result.drivesPose = deps_.tracker.drivesPose();
    result.projection = deps_.poseEstimator.projection();
    if (result.drivesPose && !result.markers.empty() && result.markers.front().found)
        result.pose = deps_.poseEstimator.estimate(result.markers.front());
    return result;
}

int Application::run() {
    if (!startUp()) {
        shutDown();
        return 1;
    }
    std::fprintf(stderr, "BaekAR: engine running. Press ESC to quit.\n");

    int exitCode = 0;
    while (!deps_.window.shouldClose()) {
        // Keep the previous frame when the source has nothing new this tick.
        deps_.source.read(frame_);
        const TrackingResult tracking = track();
        bool anyFound = false;
        for (const MarkerObservation& marker : tracking.markers) anyFound = anyFound || marker.found;
        if (anyFound) ++framesWithMarker_;

        deps_.pipeline.renderFrame(frame_, tracking, deps_.window.pointer());
        ++renderedFrames_;

        const bool lastFrame = config_.maxFrames > 0 && renderedFrames_ >= config_.maxFrames;
        if (lastFrame && !config_.screenshotPath.empty() && !saveScreenshot()) exitCode = 1;

        deps_.window.swapBuffers();
        deps_.window.pollEvents();
        if (lastFrame) deps_.window.requestClose();
    }

    shutDown();
    std::fprintf(stderr, "BaekAR: rendered %ld frame(s); a marker was found in %ld of them.\n",
                 renderedFrames_, framesWithMarker_);
    return exitCode;
}

void Application::shutDown() {
    // Workers first, then the engine, then the source they were reading.
    deps_.tracker.stop();
    deps_.pipeline.shutdown();
    deps_.source.close();
    std::fprintf(stderr, "BaekAR: tracker stopped, engine released, frame source closed.\n");
}

bool Application::saveScreenshot() {
    const cv::Mat image = deps_.window.readFramebuffer();
    if (image.empty() || !cv::imwrite(config_.screenshotPath, image)) {
        std::fprintf(stderr, "BaekAR: could not write screenshot %s\n", config_.screenshotPath.c_str());
        return false;
    }
    std::fprintf(stderr, "BaekAR: screenshot saved to %s\n", config_.screenshotPath.c_str());
    return true;
}

}  // namespace baekar
