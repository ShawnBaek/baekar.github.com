#ifndef BAEKAR_APPLICATION_PORTS_IHAND_TRACKER_H
#define BAEKAR_APPLICATION_PORTS_IHAND_TRACKER_H

#include "core/Frame.h"
#include "core/Hand.h"

namespace baekar {

// Strategy for hand tracking. GL thread only (HandyAR allocates textures).
class IHandTracker {
public:
    virtual ~IHandTracker() = default;

    virtual bool start(const Frame& firstFrame) = 0;
    virtual HandState process(const Frame& frame) = 0;
    // Image to show as the camera background this frame.
    virtual cv::Mat backgroundImage(const Frame& frame) = 0;
    // Called after the frame is rendered.
    virtual void finishFrame() = 0;
    virtual bool enabled() const = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IHAND_TRACKER_H
