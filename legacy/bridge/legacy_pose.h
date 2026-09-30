#ifndef BAEKAR_LEGACY_POSE_H
#define BAEKAR_LEGACY_POSE_H

// The 2012 marker pose (CCamera::featurePoseEstimation and its D3DX
// projection/view matrices). Plain C++ header for the C++17 layers.

#include <opencv2/core.hpp>

#include <array>
#include <memory>
#include <string>

namespace legacy_pose {

using Matrix = std::array<float, 16>;  // D3DX memory layout

class CameraPoseEstimator {
public:
    CameraPoseEstimator();
    ~CameraPoseEstimator();

    CameraPoseEstimator(const CameraPoseEstimator&) = delete;
    CameraPoseEstimator& operator=(const CameraPoseEstimator&) = delete;

    // Reads "fx fy cx cy / k1 k2 p1 p2" (calibration/calibration.txt).
    bool Load(const std::string& calibrationPath);
    // CCamera::D3DXMakeProjectionMatrix.
    Matrix Projection();
    // Sets the four marker corners (featuresResult), runs
    // featurePoseEstimation and returns CCamera::D3DXMakeViewMatrix.
    Matrix Estimate(const std::array<cv::Point2f, 4>& corners);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace legacy_pose

#endif  // BAEKAR_LEGACY_POSE_H
