#ifndef BAEKAR_APPLICATION_PORTS_IENGINE_PIPELINE_H
#define BAEKAR_APPLICATION_PORTS_IENGINE_PIPELINE_H

#include "application/AppConfig.h"
#include "core/Input.h"

namespace baekar {

// The per-frame work that still lives in the 2012 engine. This is the
// strangler seam: later stages move frame input, tracking, pose, hand
// tracking and rendering out from behind it into their own ports.
class IEnginePipeline {
public:
    virtual ~IEnginePipeline() = default;

    // Platform setup that may prompt the user (camera permission, pickers).
    virtual bool prepare(const AppConfig& config) = 0;
    // GL state, meshes and textures. Needs the current GL context.
    virtual bool initializeRenderer() = 0;
    // Opens the frame source and starts the tracking workers.
    virtual bool start() = 0;
    // Processes and renders one frame into the current back buffer.
    virtual void renderFrame(const PointerState& pointer) = 0;
    // Stops and joins every worker, then releases the frame source.
    virtual void shutdown() = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IENGINE_PIPELINE_H
