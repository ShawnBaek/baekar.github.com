#ifndef BAEKAR_ADAPTERS_POSE_LEGACY_CAMERA_POSE_ESTIMATOR_H
#define BAEKAR_ADAPTERS_POSE_LEGACY_CAMERA_POSE_ESTIMATOR_H

#include "application/ports/IPoseEstimator.h"

#include <memory>

namespace legacy_pose {
class CameraPoseEstimator;
}

namespace baekar {

// Adapter over the 2012 CCamera pose (featurePoseEstimation + D3DX matrices).
class LegacyCameraPoseEstimator final : public IPoseEstimator {
public:
    LegacyCameraPoseEstimator();
    ~LegacyCameraPoseEstimator() override;

    bool loadCalibration(const std::string& path) override;
    Mat4 projection() const override { return projection_; }
    Pose estimate(const MarkerObservation& marker) override;

private:
    std::unique_ptr<legacy_pose::CameraPoseEstimator> camera_;
    Mat4 projection_ = Mat4::identity();
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_POSE_LEGACY_CAMERA_POSE_ESTIMATOR_H
