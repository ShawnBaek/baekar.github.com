#ifndef BAEKAR_ADAPTERS_HAND_LANDMARK_HAND_TRACKER_H
#define BAEKAR_ADAPTERS_HAND_LANDMARK_HAND_TRACKER_H

#include "application/ports/IHandLandmarkDetector.h"
#include "application/ports/IHandTracker.h"

#include <cstdint>
#include <memory>

namespace baekar {

struct PinchSettings {
    // Thumb-tip to index-tip distance divided by the palm length (wrist to
    // middle-finger base). The gap between the two thresholds keeps a pinch
    // from flickering on and off at the boundary.
    float startRatio = 0.25f;
    float endRatio = 0.40f;
    // Joints below this confidence count as not located.
    float minJointConfidence = 0.3f;
};

// Pinch recognizer with hysteresis: starts below startRatio, holds until
// the ratio rises above endRatio or the hand is lost.
class PinchGesture {
public:
    explicit PinchGesture(PinchSettings settings = {});

    // Returns whether the hand is pinching after this frame.
    bool update(const HandLandmarks& hand);
    bool pinching() const { return pinching_; }
    void reset() { pinching_ = false; }

    // Negative when the thumb tip, index tip, wrist or middle-finger base
    // is not located, or the palm has no length.
    float ratio(const HandLandmarks& hand) const;

private:
    PinchSettings settings_;
    bool pinching_ = false;
};

// IHandTracker over any 21-joint detector. The pinch is the point/click
// gesture (validPose) and the index fingertip is the pointer, so
// InteractionController picks on pinch and drags while it is held.
class LandmarkHandTracker final : public IHandTracker {
public:
    explicit LandmarkHandTracker(std::unique_ptr<IHandLandmarkDetector> detector, PinchSettings settings = {});

    bool start(const Frame&) override { return true; }
    // Detects once per new frame; a repeated frame (same sequence) returns
    // the previous result.
    HandState process(const Frame& frame) override;
    // The camera frame with the detected skeleton drawn on it.
    cv::Mat backgroundImage(const Frame& frame) override;
    void finishFrame() override {}
    bool enabled() const override { return true; }

    const HandLandmarks& lastLandmarks() const { return landmarks_; }
    const IHandLandmarkDetector& detector() const { return *detector_; }

private:
    std::unique_ptr<IHandLandmarkDetector> detector_;
    PinchGesture pinch_;
    HandLandmarks landmarks_;
    HandState state_;
    std::uint64_t lastSequence_ = 0;
};

// Draws the hand skeleton; the thumb and index tips turn red while pinching.
void drawHandLandmarks(cv::Mat& image, const HandLandmarks& hand, bool pinching, float minJointConfidence = 0.3f);

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_HAND_LANDMARK_HAND_TRACKER_H
