// PinchGesture and LandmarkHandTracker against a scripted landmark
// detector: pinch hysteresis, joint confidence, window coordinates.

#include "adapters/hand/LandmarkHandTracker.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <utility>

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

void set(baekar::HandLandmarks& hand, baekar::HandJoint joint, float x, float y) {
    hand.points[static_cast<int>(joint)] = cv::Point2f(x, y);
}

// An upright hand with a 100 px palm (wrist at y=300, middle base at
// y=200) and the thumb tip `gap` px from the index tip.
baekar::HandLandmarks handWithGap(float gap) {
    baekar::HandLandmarks hand;
    hand.found = true;
    hand.confidence = 0.9f;
    hand.jointConfidence.fill(0.9f);
    for (int i = 0; i < baekar::kHandJointCount; ++i) hand.points[i] = cv::Point2f(100, 250);
    set(hand, baekar::HandJoint::Wrist, 100, 300);
    set(hand, baekar::HandJoint::MiddleMcp, 100, 200);
    set(hand, baekar::HandJoint::IndexTip, 120, 120);
    set(hand, baekar::HandJoint::ThumbTip, 120 - gap, 120);
    return hand;
}

class ScriptedDetector final : public baekar::IHandLandmarkDetector {
public:
    baekar::HandLandmarks next;
    baekar::HandLandmarks detect(const cv::Mat&) override { return next; }
    std::string describe() const override { return "scripted"; }
};

baekar::Frame frameOfSize(int width, int height) {
    baekar::Frame frame;
    frame.bgr = cv::Mat(height, width, CV_8UC3, cv::Scalar(30, 30, 30));
    frame.sequence = 1;
    return frame;
}

}  // namespace

int main() {
    using baekar::HandJoint;
    {
        baekar::PinchGesture pinch;
        expect(std::fabs(pinch.ratio(handWithGap(50)) - 0.5f) < 1e-4f, "ratio is tip gap over palm length");
        expect(pinch.ratio(baekar::HandLandmarks()) < 0, "no hand has no ratio");

        // Open, close, hold inside the hysteresis band, release, and stay
        // released inside the band.
        expect(!pinch.update(handWithGap(50)), "open hand is not a pinch");
        expect(pinch.update(handWithGap(20)), "tips together start a pinch");
        expect(pinch.update(handWithGap(35)), "pinch holds between the thresholds");
        expect(!pinch.update(handWithGap(45)), "tips apart end the pinch");
        expect(!pinch.update(handWithGap(35)), "no new pinch between the thresholds");

        expect(pinch.update(handWithGap(10)), "pinch again");
        expect(!pinch.update(baekar::HandLandmarks()), "losing the hand ends the pinch");

        baekar::HandLandmarks unsure = handWithGap(10);
        unsure.jointConfidence[static_cast<int>(HandJoint::ThumbTip)] = 0.1f;
        expect(!pinch.update(unsure), "an unlocated thumb tip is not a pinch");

        baekar::HandLandmarks flat = handWithGap(10);
        set(flat, HandJoint::MiddleMcp, 100, 300);
        expect(pinch.ratio(flat) < 0, "a zero-length palm has no ratio");
    }
    {
        auto detector = std::make_unique<ScriptedDetector>();
        ScriptedDetector* script = detector.get();
        baekar::LandmarkHandTracker tracker(std::move(detector));
        // A 1280x960 frame maps to the 640x480 window at half scale.
        baekar::Frame frame = frameOfSize(1280, 960);

        script->next = handWithGap(50);
        baekar::HandState state = tracker.process(frame);
        expect(state.handFound && !state.validPose, "open hand: found, no gesture");
        expect(!state.hasPose3d, "landmark tracker has no 6DoF hand pose");
        expect(state.indexFingertip == cv::Point2f(60, 60), "index tip in window pixels");
        const cv::Mat drawn = tracker.backgroundImage(frame);
        expect(drawn.data != frame.bgr.data && cv::norm(drawn, frame.bgr, cv::NORM_INF) > 0,
               "skeleton drawn on a copy of the frame");

        script->next = handWithGap(10);
        state = tracker.process(frame);
        expect(!state.validPose, "a repeated frame keeps the previous result");
        ++frame.sequence;
        state = tracker.process(frame);
        expect(state.validPose, "pinch is the point/click gesture");

        script->next = baekar::HandLandmarks();
        ++frame.sequence;
        state = tracker.process(frame);
        expect(!state.handFound && !state.validPose, "no hand: no gesture");
        expect(tracker.backgroundImage(frame).data == frame.bgr.data, "no hand: frame shown unchanged");
        expect(!tracker.process(baekar::Frame()).handFound, "empty frame is skipped");
    }

    if (failures == 0) std::fprintf(stderr, "landmark_hand_tracker_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
