#ifndef BAEKAR_APPLICATION_PORTS_IRENDERER_H
#define BAEKAR_APPLICATION_PORTS_IRENDERER_H

#include "core/Hand.h"
#include "core/Tracking.h"

#include <opencv2/core.hpp>

#include <vector>

namespace baekar {

// Draws one frame. GL thread only. The 2012 fixed-function OpenGL renderer
// implements it today; a Metal renderer would be a second implementation.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual bool initialize() = 0;
    virtual void beginFrame() = 0;
    virtual void drawBackground(const cv::Mat& bgr) = 0;
    // Outlines and labels for markers that do not drive the 3D pose.
    virtual void drawOutlines(const std::vector<MarkerObservation>& markers) = 0;
    virtual void setProjection(const Mat4& projection) = 0;
    // 3D overlay for the pose-driving marker.
    virtual void drawMarkerAnchor(const MarkerObservation& marker, const Mat4& projection,
                                  const Pose& pose) = 0;
    virtual void drawHand(const HandState& hand) = 0;
    virtual void endFrame() = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IRENDERER_H
