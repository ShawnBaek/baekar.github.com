#include "adapters/hand/HandTrackers.h"

#include "legacy_hand.h"

#include <utility>

namespace baekar {

HandyArHandTracker::HandyArHandTracker(std::string calibrationPath, std::string fingertipPath)
    : calibrationPath_(std::move(calibrationPath)),
      fingertipPath_(std::move(fingertipPath)),
      tracker_(std::make_unique<legacy_hand::HandTracker>()) {}

HandyArHandTracker::~HandyArHandTracker() = default;

bool HandyArHandTracker::start(const Frame& firstFrame) {
    return tracker_->Initialize(firstFrame.bgr, calibrationPath_, fingertipPath_);
}

HandState HandyArHandTracker::process(const Frame& frame) {
    const legacy_hand::State state = tracker_->Process(frame.bgr, frame.tickCount);
    HandState hand;
    hand.validPose = state.validPose;
    hand.indexFingertip = state.indexFingertip;
    hand.projection.m = state.projection;
    hand.view.m = state.view;
    return hand;
}

cv::Mat HandyArHandTracker::backgroundImage(const Frame& frame) {
    // HandyAR's display image: the camera frame with its skin-region overlay.
    cv::Mat view = tracker_->DebugView();
    return view.empty() ? frame.bgr : view;
}

void HandyArHandTracker::finishFrame() { tracker_->FinishFrame(); }

}  // namespace baekar
