#ifndef BAEKAR_CORE_HAND_H
#define BAEKAR_CORE_HAND_H

#include "core/Tracking.h"

#include <opencv2/core.hpp>

#include <array>

namespace baekar {

// The 21 joints of one hand, in the order MediaPipe and Apple Vision share:
// the wrist, then four joints per finger from the base to the tip.
enum class HandJoint : int {
    Wrist = 0,
    ThumbCmc, ThumbMp, ThumbIp, ThumbTip,
    IndexMcp, IndexPip, IndexDip, IndexTip,
    MiddleMcp, MiddlePip, MiddleDip, MiddleTip,
    RingMcp, RingPip, RingDip, RingTip,
    LittleMcp, LittlePip, LittleDip, LittleTip,
};
constexpr int kHandJointCount = 21;

// One hand's joints in one image, from a learned hand-pose detector.
struct HandLandmarks {
    bool found = false;
    float confidence = 0.0f;                             // whole-hand score, 0..1
    std::array<cv::Point2f, kHandJointCount> points{};   // image pixels, top-left origin
    std::array<float, kHandJointCount> jointConfidence{};  // 0: joint not located

    const cv::Point2f& operator[](HandJoint joint) const { return points[static_cast<int>(joint)]; }
    float confidenceOf(HandJoint joint) const { return jointConfidence[static_cast<int>(joint)]; }
};

// Hand tracking result for one frame.
struct HandState {
    // The point/click gesture is held. HandyAR: five fingertips found.
    // Learned trackers: thumb and index fingertips pinched together.
    bool validPose = false;
    cv::Point2f indexFingertip;   // window pixels
    bool handFound = false;       // a hand is visible, gesturing or not
    bool hasPose3d = false;       // projection and view hold a 6DoF hand pose
    Mat4 projection;
    Mat4 view;
};

}  // namespace baekar

#endif  // BAEKAR_CORE_HAND_H
