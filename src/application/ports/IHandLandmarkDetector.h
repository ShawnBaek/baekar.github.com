#ifndef BAEKAR_APPLICATION_PORTS_IHAND_LANDMARK_DETECTOR_H
#define BAEKAR_APPLICATION_PORTS_IHAND_LANDMARK_DETECTOR_H

#include "core/Hand.h"

#include <opencv2/core.hpp>

#include <string>

namespace baekar {

// Strategy for finding one hand's 21 joints in an image: Apple Vision on
// macOS/iOS today; a learned model through IInferenceEngine on other
// platforms later. LandmarkHandTracker turns the joints into a gesture.
class IHandLandmarkDetector {
public:
    virtual ~IHandLandmarkDetector() = default;

    // `bgr` is CV_8UC3. Points come back in its pixel coordinates.
    virtual HandLandmarks detect(const cv::Mat& bgr) = 0;
    virtual std::string describe() const = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IHAND_LANDMARK_DETECTOR_H
