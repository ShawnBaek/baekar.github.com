// Wraps the 2012 CCamera so each estimator owns its own camera instead of
// sharing the `camera` global in EngineMain.cpp.

#include "legacy_pose.h"

#include "calibration/Camera.h"

namespace legacy_pose {
namespace {

Matrix toMatrix(const D3DXMATRIXA16& in) {
    Matrix out;
    for (int i = 0; i < 16; ++i) out[i] = in[i];
    return out;
}

}  // namespace

struct CameraPoseEstimator::Impl {
    CCamera camera;
};

CameraPoseEstimator::CameraPoseEstimator() : impl_(new Impl) {}

CameraPoseEstimator::~CameraPoseEstimator() = default;

bool CameraPoseEstimator::Load(const std::string& calibrationPath) {
    return impl_->camera.load(calibrationPath.c_str());
}

Matrix CameraPoseEstimator::Projection() {
    D3DXMATRIXA16 projection;
    impl_->camera.D3DXMakeProjectionMatrix(&projection);
    return toMatrix(projection);
}

Matrix CameraPoseEstimator::Estimate(const std::array<cv::Point2f, 4>& corners) {
    Results& features = impl_->camera.featuresResult;
    for (int i = 0; i < 4; ++i) {
        features.vertex[i].x = corners[i].x;
        features.vertex[i].y = corners[i].y;
    }
    // Same center formula the 2012 matching/tracking threads used.
    features.center.x = features.vertex[0].x + ((features.vertex[1].x - features.vertex[0].x) / 2);
    features.center.y = features.vertex[0].y + ((features.vertex[3].y - features.vertex[0].y) / 2);

    impl_->camera.featurePoseEstimation();
    D3DXMATRIXA16 view;
    impl_->camera.D3DXMakeViewMatrix(&view);
    return toMatrix(view);
}

}  // namespace legacy_pose
