#include "adapters/legacy/LegacyEnginePipeline.h"

#include "legacy_engine.h"

namespace baekar {

LegacyEnginePipeline::LegacyEnginePipeline(int argc, char** argv) : argc_(argc), argv_(argv) {}

bool LegacyEnginePipeline::prepare(const AppConfig& config) {
    legacy_engine::Options options;
    options.cameraIndex = config.cameraIndex;
    options.markerImage = config.markerImage;
    options.simulatedMarkers = config.simulatedMarkers;
    switch (config.tracker) {
    case TrackerKind::Legacy: options.multiMarkerTracker = false; break;
    case TrackerKind::Multi: options.multiMarkerTracker = true; break;
    case TrackerKind::Auto: options.multiMarkerTracker = config.simulatedMarkers.size() > 1; break;
    }
    options.handTracking = config.handTracking;
    options.interactive = config.interactive;
    options.windowCapture = config.windowCapture;
    return legacy_engine::Prepare(options, argc_, argv_);
}

bool LegacyEnginePipeline::initializeRenderer() { return legacy_engine::InitializeRenderer(); }

bool LegacyEnginePipeline::start() {
    started_ = legacy_engine::Start();
    return started_;
}

void LegacyEnginePipeline::renderFrame(const PointerState& pointer) {
    legacy_engine::Pointer legacyPointer;
    legacyPointer.x = pointer.x;
    legacyPointer.y = pointer.y;
    legacyPointer.leftDown = pointer.leftDown;
    legacy_engine::RenderFrame(legacyPointer);
}

void LegacyEnginePipeline::shutdown() {
    legacy_engine::Shutdown();
    started_ = false;
}

}  // namespace baekar
