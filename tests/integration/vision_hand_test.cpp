// Apple Vision hand pose on the thesis hand photo (assets/image/hand.JPG):
// an open, upright right hand that HandyAR finds no pose in.

#include "adapters/hand/LandmarkHandTracker.h"
#include "platform/macos/VisionHandLandmarkDetector.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

}  // namespace

int main() {
    using baekar::HandJoint;
    const cv::Mat photo = cv::imread(std::string(BAEKAR_SOURCE_DIR) + "/assets/image/hand.JPG");
    if (photo.empty()) {
        std::fprintf(stderr, "FAIL: cannot read assets/image/hand.JPG\n");
        return 1;
    }
    // The engine's frame size, as the app would see it.
    const cv::Mat frame = baekar::toEngineFrameSize(photo);

    baekar::VisionHandLandmarkDetector detector;
    const baekar::HandLandmarks hand = detector.detect(frame);
    expect(hand.found, "Vision finds the hand");
    if (hand.found) {
        int located = 0;
        for (float c : hand.jointConfidence) located += c >= 0.3f;
        std::fprintf(stderr, "vision_hand_test: confidence %.2f, %d of 21 joints located\n", hand.confidence, located);
        expect(located >= 18, "most joints located");
        const cv::Rect bounds(0, 0, frame.cols, frame.rows);
        for (const cv::Point2f& p : hand.points) expect(bounds.contains(p), "joints inside the image");
        // Upright hand: every fingertip is above the wrist and its own base.
        const std::pair<HandJoint, HandJoint> fingers[] = {
            {HandJoint::IndexTip, HandJoint::IndexMcp}, {HandJoint::MiddleTip, HandJoint::MiddleMcp},
            {HandJoint::RingTip, HandJoint::RingMcp}, {HandJoint::LittleTip, HandJoint::LittleMcp}};
        for (const auto& [tip, base] : fingers) {
            expect(hand[tip].y < hand[base].y, "fingertip above its base");
            expect(hand[tip].y < hand[HandJoint::Wrist].y, "fingertip above the wrist");
        }
        // The middle finger is the tallest in the photo (near x=155 of 282).
        const cv::Point2f middle = hand[HandJoint::MiddleTip];
        expect(middle.x > frame.cols * 0.45f && middle.x < frame.cols * 0.65f && middle.y < frame.rows * 0.2f,
               "middle fingertip where the photo has it");

        baekar::PinchGesture pinch;
        std::fprintf(stderr, "vision_hand_test: pinch ratio %.2f\n", pinch.ratio(hand));
        expect(!pinch.update(hand), "an open hand is not a pinch");
    }
    expect(!detector.detect(cv::Mat(480, 640, CV_8UC3, cv::Scalar(40, 40, 40))).found, "no hand in a blank frame");

    if (failures == 0) std::fprintf(stderr, "vision_hand_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
