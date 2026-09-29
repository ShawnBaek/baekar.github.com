#ifndef BAEKAR_APPLICATION_PORTS_IENGINE_PIPELINE_H
#define BAEKAR_APPLICATION_PORTS_IENGINE_PIPELINE_H

#include "application/AppConfig.h"
#include "core/Frame.h"
#include "core/Input.h"
#include "core/Tracking.h"

namespace baekar {

// The per-frame work that still lives in the 2012 engine: hand tracking,
// rendering and interaction. This is the strangler seam; frame input
// (IFrameSource), marker tracking (IMarkerTracker) and the marker pose
// (IPoseEstimator) have already moved out from behind it.
class IEnginePipeline {
public:
    virtual ~IEnginePipeline() = default;

    // Hand tracking switch and the optional macOS window capture.
    virtual bool prepare(const AppConfig& config) = 0;
    // GL state, meshes and textures. Needs the current GL context.
    virtual bool initializeRenderer() = 0;
    // Hand tracking initialization.
    virtual bool start(const Frame& firstFrame) = 0;
    // Hand tracking, rendering and interaction for one frame.
    virtual void renderFrame(const Frame& frame, const TrackingResult& tracking,
                             const PointerState& pointer) = 0;
    virtual void shutdown() = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IENGINE_PIPELINE_H
