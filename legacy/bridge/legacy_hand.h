#ifndef BAEKAR_LEGACY_HAND_H
#define BAEKAR_LEGACY_HAND_H

// HandyAR fingertip tracking (FingertipPoseEstimation) with its own
// instance instead of the gFingertipPoseEstimation global.

#include <opencv2/core.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace legacy_hand {

using Matrix = std::array<float, 16>;  // 2012 D3DX layout

struct State {
    bool validPose = false;
    cv::Point2f indexFingertip;  // window pixels (640x480)
    Matrix projection{};
    Matrix view{};
};

class HandTracker {
public:
    HandTracker();
    ~HandTracker();

    HandTracker(const HandTracker&) = delete;
    HandTracker& operator=(const HandTracker&) = delete;

    // Loads fingertip coordinates and initializes with the first frame
    // (calls glGenTextures: GL thread only).
    bool Initialize(const cv::Mat& firstFrame, const std::string& calibrationPath,
                    const std::string& fingertipPath);
    // OnCapture only: keeps the debug view current without hand tracking.
    void Capture(const cv::Mat& bgr, std::int64_t tickCount);
    // OnCapture + OnProcess + fingertip pose matrices.
    State Process(const cv::Mat& bgr, std::int64_t tickCount);
    // Ends HandyAR's per-frame timing (TickCountEnd/NewLine).
    void FinishFrame();
    // HandyAR's display image (the 2012 camera background).
    cv::Mat DebugView();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace legacy_hand

#endif  // BAEKAR_LEGACY_HAND_H
