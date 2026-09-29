#ifndef BAEKAR_APPLICATION_PORTS_IPOSE_ESTIMATOR_H
#define BAEKAR_APPLICATION_PORTS_IPOSE_ESTIMATOR_H

#include "core/Tracking.h"

#include <string>

namespace baekar {

// Strategy for turning four marker corners into a camera pose.
class IPoseEstimator {
public:
    virtual ~IPoseEstimator() = default;

    virtual bool loadCalibration(const std::string& path) = 0;
    // Projection matrix for the calibrated camera (constant per run).
    virtual Mat4 projection() const = 0;
    virtual Pose estimate(const MarkerObservation& marker) = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IPOSE_ESTIMATOR_H
