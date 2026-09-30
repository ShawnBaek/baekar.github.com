#include "adapters/pose/LegacyCameraPoseEstimator.h"

#include "legacy_pose.h"

namespace baekar {

LegacyCameraPoseEstimator::LegacyCameraPoseEstimator()
    : camera_(std::make_unique<legacy_pose::CameraPoseEstimator>()) {}

LegacyCameraPoseEstimator::~LegacyCameraPoseEstimator() = default;

bool LegacyCameraPoseEstimator::loadCalibration(const std::string& path) {
    if (!camera_->Load(path)) return false;
    // The intrinsics are fixed for the run, so the projection is computed
    // once (the 2012 loop rebuilt and logged it every frame).
    projection_.m = camera_->Projection();
    return true;
}

Pose LegacyCameraPoseEstimator::estimate(const MarkerObservation& marker) {
    Pose pose;
    if (!marker.found) return pose;
    pose.view.m = camera_->Estimate(marker.poseCorners);
    pose.valid = true;
    return pose;
}

}  // namespace baekar
