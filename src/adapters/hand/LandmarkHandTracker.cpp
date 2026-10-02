#include "adapters/hand/LandmarkHandTracker.h"

#include <opencv2/imgproc.hpp>

#include <cmath>
#include <utility>

namespace baekar {
namespace {

float distance(const cv::Point2f& a, const cv::Point2f& b) { return std::hypot(a.x - b.x, a.y - b.y); }

// Pairs of joints joined by a bone: each finger from the wrist to its tip,
// plus the knuckle line across the palm.
constexpr std::pair<HandJoint, HandJoint> kBones[] = {
    {HandJoint::Wrist, HandJoint::ThumbCmc},     {HandJoint::ThumbCmc, HandJoint::ThumbMp},
    {HandJoint::ThumbMp, HandJoint::ThumbIp},    {HandJoint::ThumbIp, HandJoint::ThumbTip},
    {HandJoint::Wrist, HandJoint::IndexMcp},     {HandJoint::IndexMcp, HandJoint::IndexPip},
    {HandJoint::IndexPip, HandJoint::IndexDip},  {HandJoint::IndexDip, HandJoint::IndexTip},
    {HandJoint::MiddleMcp, HandJoint::MiddlePip}, {HandJoint::MiddlePip, HandJoint::MiddleDip},
    {HandJoint::MiddleDip, HandJoint::MiddleTip}, {HandJoint::RingMcp, HandJoint::RingPip},
    {HandJoint::RingPip, HandJoint::RingDip},    {HandJoint::RingDip, HandJoint::RingTip},
    {HandJoint::Wrist, HandJoint::LittleMcp},    {HandJoint::LittleMcp, HandJoint::LittlePip},
    {HandJoint::LittlePip, HandJoint::LittleDip}, {HandJoint::LittleDip, HandJoint::LittleTip},
    {HandJoint::IndexMcp, HandJoint::MiddleMcp}, {HandJoint::MiddleMcp, HandJoint::RingMcp},
    {HandJoint::RingMcp, HandJoint::LittleMcp},
};

}  // namespace

PinchGesture::PinchGesture(PinchSettings settings) : settings_(settings) {}

float PinchGesture::ratio(const HandLandmarks& hand) const {
    if (!hand.found) return -1.0f;
    for (HandJoint joint : {HandJoint::ThumbTip, HandJoint::IndexTip, HandJoint::Wrist, HandJoint::MiddleMcp})
        if (hand.confidenceOf(joint) < settings_.minJointConfidence) return -1.0f;
    const float palm = distance(hand[HandJoint::Wrist], hand[HandJoint::MiddleMcp]);
    if (palm < 1.0f) return -1.0f;
    return distance(hand[HandJoint::ThumbTip], hand[HandJoint::IndexTip]) / palm;
}

bool PinchGesture::update(const HandLandmarks& hand) {
    const float r = ratio(hand);
    if (r < 0.0f) pinching_ = false;
    else if (pinching_) pinching_ = r <= settings_.endRatio;
    else pinching_ = r < settings_.startRatio;
    return pinching_;
}

LandmarkHandTracker::LandmarkHandTracker(std::unique_ptr<IHandLandmarkDetector> detector, PinchSettings settings)
    : detector_(std::move(detector)), pinch_(settings) {}

HandState LandmarkHandTracker::process(const Frame& frame) {
    if (frame.sequence != 0 && frame.sequence == lastSequence_) return state_;
    lastSequence_ = frame.sequence;
    HandState state;
    landmarks_ = frame.bgr.empty() ? HandLandmarks() : detector_->detect(frame.bgr);
    state.handFound = landmarks_.found;
    state.validPose = pinch_.update(landmarks_);
    if (landmarks_.found) {
        // Frame pixels to window pixels (the window shows the frame stretched).
        const cv::Point2f tip = landmarks_[HandJoint::IndexTip];
        state.indexFingertip = cv::Point2f(tip.x * kFrameWidth / frame.bgr.cols, tip.y * kFrameHeight / frame.bgr.rows);
    }
    state_ = state;
    return state;
}

cv::Mat LandmarkHandTracker::backgroundImage(const Frame& frame) {
    if (!landmarks_.found || frame.bgr.empty()) return frame.bgr;
    cv::Mat view = frame.bgr.clone();
    drawHandLandmarks(view, landmarks_, pinch_.pinching());
    return view;
}

void drawHandLandmarks(cv::Mat& image, const HandLandmarks& hand, bool pinching, float minJointConfidence) {
    if (!hand.found) return;
    auto located = [&](HandJoint joint) { return hand.confidenceOf(joint) >= minJointConfidence; };
    const cv::Scalar bone(80, 220, 80), joint(255, 255, 255), pinch(40, 40, 255);
    for (const auto& [a, b] : kBones)
        if (located(a) && located(b)) cv::line(image, hand[a], hand[b], bone, 2, cv::LINE_AA);
    for (int i = 0; i < kHandJointCount; ++i) {
        const auto j = static_cast<HandJoint>(i);
        if (!located(j)) continue;
        const bool tip = j == HandJoint::ThumbTip || j == HandJoint::IndexTip;
        cv::circle(image, hand[j], tip ? 6 : 3, pinching && tip ? pinch : joint, cv::FILLED, cv::LINE_AA);
    }
}

}  // namespace baekar
