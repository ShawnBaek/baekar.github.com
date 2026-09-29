#include "adapters/legacy/LegacyEnginePipeline.h"

#include "legacy_engine.h"

namespace baekar {
namespace {

legacy_engine::FrameInput toLegacy(const Frame& frame) {
    legacy_engine::FrameInput input;
    input.bgr = frame.bgr;
    input.sequence = frame.sequence;
    input.tickCount = frame.tickCount;
    input.live = !frame.placeholder;
    return input;
}

legacy_engine::TrackingInput toLegacy(const TrackingResult& tracking) {
    legacy_engine::TrackingInput input;
    for (const MarkerObservation& observation : tracking.markers) {
        legacy_engine::TrackedMarker marker;
        marker.index = observation.markerIndex;
        marker.name = observation.name;
        marker.found = observation.found;
        marker.tracking = observation.tracking;
        marker.outline = observation.detectionCorners;
        marker.inliers = observation.inliers;
        marker.trackedPoints = observation.trackedPoints;
        input.markers.push_back(marker);
    }
    input.drivesPose = tracking.drivesPose;
    input.projection = tracking.projection.m;
    input.poseValid = tracking.pose.valid;
    input.view = tracking.pose.view.m;
    return input;
}

}  // namespace

LegacyEnginePipeline::LegacyEnginePipeline(int argc, char** argv) : argc_(argc), argv_(argv) {}

bool LegacyEnginePipeline::prepare(const AppConfig& config) {
    legacy_engine::Options options;
    options.handTracking = config.handTracking;
    options.cameraInput = !config.usesSimulation() && config.replayDirectory.empty();
    options.windowCapture = config.windowCapture;
    return legacy_engine::Prepare(options, argc_, argv_);
}

bool LegacyEnginePipeline::initializeRenderer() { return legacy_engine::InitializeRenderer(); }

bool LegacyEnginePipeline::start(const Frame& firstFrame) {
    started_ = legacy_engine::Start(toLegacy(firstFrame));
    return started_;
}

void LegacyEnginePipeline::renderFrame(const Frame& frame, const TrackingResult& tracking,
                                       const PointerState& pointer) {
    legacy_engine::Pointer legacyPointer;
    legacyPointer.x = pointer.x;
    legacyPointer.y = pointer.y;
    legacyPointer.leftDown = pointer.leftDown;
    legacy_engine::RenderFrame(toLegacy(frame), toLegacy(tracking), legacyPointer);
}

void LegacyEnginePipeline::shutdown() {
    legacy_engine::Shutdown();
    started_ = false;
}

}  // namespace baekar
