#include "application/Application.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <utility>

namespace baekar {

Application::Application(RunSettings settings, Dependencies dependencies)
    : settings_(std::move(settings)), deps_(dependencies), interaction_(dependencies.scene) {}

bool Application::startUp() {
    // GL state first: HandyAR allocates textures during start().
    if (!deps_.renderer.initialize()) {
        std::fprintf(stderr, "BaekAR: renderer initialization failed.\n");
        return false;
    }
    std::fprintf(stderr, "BaekAR: frame source: %s\n", deps_.source.describe().c_str());
    if (!deps_.source.read(frame_)) {
        std::fprintf(stderr, "BaekAR: the frame source produced no first frame.\n");
        return false;
    }
    if (frame_.placeholder) std::fprintf(stderr, "Camera unavailable — running with dummy frame.\n");
    if (!deps_.poseEstimator.loadCalibration(settings_.calibrationPath))
        std::fprintf(stderr, "BaekAR: could not read calibration %s\n", settings_.calibrationPath.c_str());
    if (!deps_.handTracker.start(frame_)) {
        std::fprintf(stderr, "BaekAR: hand tracker start failed.\n");
        return false;
    }
    std::fprintf(stderr, "BaekAR: marker tracker: %s\n", deps_.tracker.describe().c_str());
    for (const std::string& marker : settings_.markerImages)
        std::fprintf(stderr, "  marker %s\n", marker.c_str());
    if (!deps_.tracker.start(settings_.markerImages, frame_)) {
        std::fprintf(stderr, "BaekAR: marker tracker start failed.\n");
        if (settings_.markerImages.size() > 1)
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

void Application::renderFrame(const TrackingResult& tracking, const HandState& hand) {
    IRenderer& renderer = deps_.renderer;
    renderer.beginFrame();
    renderer.drawBackground(deps_.handTracker.backgroundImage(frame_));
    if (!tracking.drivesPose) renderer.drawOutlines(tracking.markers);
    renderer.setProjection(tracking.projection);
    if (tracking.drivesPose && tracking.pose.valid) {
        renderer.drawMarkerAnchor(tracking.markers.front(), tracking.projection, tracking.pose);
        deps_.scene.render(tracking.projection, tracking.pose.view);
    }
    if (deps_.handTracker.enabled()) renderer.drawHand(hand);
    deps_.handTracker.finishFrame();
    renderer.endFrame();
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
        // Mouse picking runs before drawing, against the last drawn scene.
        interaction_.onPointer(deps_.window.pointer());

        const TrackingResult tracking = track();
        const HandState hand = deps_.handTracker.process(frame_);
        if (deps_.windowTexture && deps_.windowTexture->latest(windowPixels_))
            deps_.scene.setWindowTexture(windowPixels_);

        renderFrame(tracking, hand);
        if (deps_.handTracker.enabled()) interaction_.onHand(hand);

        bool anyFound = false;
        for (const MarkerObservation& marker : tracking.markers) anyFound = anyFound || marker.found;
        if (anyFound) ++framesWithMarker_;
        if (hand.validPose) ++framesWithHandPose_;
        ++renderedFrames_;

        const bool lastFrame = settings_.maxFrames > 0 && renderedFrames_ >= settings_.maxFrames;
        if (lastFrame && !settings_.screenshotPath.empty() && !saveScreenshot()) exitCode = 1;

        deps_.window.swapBuffers();
        deps_.window.pollEvents();
        if (lastFrame) deps_.window.requestClose();
    }

    shutDown();
    std::fprintf(stderr,
                 "BaekAR: rendered %ld frame(s); marker found in %ld, hand pose in %ld.\n",
                 renderedFrames_, framesWithMarker_, framesWithHandPose_);
    return exitCode;
}

void Application::shutDown() {
    // Workers first, then the sources they were reading.
    deps_.tracker.stop();
    if (deps_.windowTexture) deps_.windowTexture->close();
    deps_.source.close();
    std::fprintf(stderr, "BaekAR: tracker stopped, frame source closed.\n");
}

bool Application::saveScreenshot() {
    const cv::Mat image = deps_.window.readFramebuffer();
    if (image.empty() || !cv::imwrite(settings_.screenshotPath, image)) {
        std::fprintf(stderr, "BaekAR: could not write screenshot %s\n", settings_.screenshotPath.c_str());
        return false;
    }
    std::fprintf(stderr, "BaekAR: screenshot saved to %s\n", settings_.screenshotPath.c_str());
    return true;
}

}  // namespace baekar
