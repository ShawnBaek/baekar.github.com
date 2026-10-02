#ifndef BAEKAR_PLATFORM_MACOS_VISION_HAND_LANDMARK_DETECTOR_H
#define BAEKAR_PLATFORM_MACOS_VISION_HAND_LANDMARK_DETECTOR_H

#include "application/ports/IHandLandmarkDetector.h"

#include <memory>

namespace baekar {

// Apple Vision hand pose (VNDetectHumanHandPoseRequest), on device. Finds
// the most prominent hand. Synchronous: call it from one thread.
class VisionHandLandmarkDetector final : public IHandLandmarkDetector {
public:
    VisionHandLandmarkDetector();
    ~VisionHandLandmarkDetector() override;

    VisionHandLandmarkDetector(const VisionHandLandmarkDetector&) = delete;
    VisionHandLandmarkDetector& operator=(const VisionHandLandmarkDetector&) = delete;

    HandLandmarks detect(const cv::Mat& bgr) override;
    std::string describe() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace baekar

#endif  // BAEKAR_PLATFORM_MACOS_VISION_HAND_LANDMARK_DETECTOR_H
