#ifndef BAEKAR_CORE_HAND_H
#define BAEKAR_CORE_HAND_H

#include "core/Tracking.h"

#include <opencv2/core.hpp>

namespace baekar {

// Fingertip hand tracking result for one frame.
struct HandState {
    bool validPose = false;       // five fingertips found: the point/click gesture
    cv::Point2f indexFingertip;   // window pixels
    Mat4 projection;
    Mat4 view;
};

}  // namespace baekar

#endif  // BAEKAR_CORE_HAND_H
