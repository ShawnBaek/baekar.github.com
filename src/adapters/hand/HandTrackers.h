#ifndef BAEKAR_ADAPTERS_HAND_HAND_TRACKERS_H
#define BAEKAR_ADAPTERS_HAND_HAND_TRACKERS_H

#include "application/ports/IHandTracker.h"

#include <memory>
#include <string>

namespace legacy_hand {
class HandTracker;
}

namespace baekar {

// Adapter over HandyAR (skin-color hand region, fingertips, 6DoF hand pose).
class HandyArHandTracker final : public IHandTracker {
public:
    HandyArHandTracker(std::string calibrationPath, std::string fingertipPath);
    ~HandyArHandTracker() override;

    bool start(const Frame& firstFrame) override;
    HandState process(const Frame& frame) override;
    cv::Mat backgroundImage(const Frame& frame) override;
    void finishFrame() override;
    bool enabled() const override { return true; }

private:
    std::string calibrationPath_;
    std::string fingertipPath_;
    std::unique_ptr<legacy_hand::HandTracker> tracker_;
};

// Null Object for --no-hand: no hand pose, the camera frame as background.
class DisabledHandTracker final : public IHandTracker {
public:
    bool start(const Frame&) override { return true; }
    HandState process(const Frame&) override { return HandState(); }
    cv::Mat backgroundImage(const Frame& frame) override { return frame.bgr; }
    void finishFrame() override {}
    bool enabled() const override { return false; }
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_HAND_HAND_TRACKERS_H
