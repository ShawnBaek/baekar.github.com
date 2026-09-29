#ifndef BAEKAR_ADAPTERS_TRACKING_MARKER_TRACKERS_H
#define BAEKAR_ADAPTERS_TRACKING_MARKER_TRACKERS_H

#include "application/ports/IMarkerTracker.h"

#include <memory>

class MultiMarkerDetector;
namespace legacy_tracking {
class SingleMarkerTracker;
}

namespace baekar {

// Adapter over the 2012 single-marker pipeline (BRISK matching + NCC
// tracking threads). Its observations drive the 3D pose.
class LegacySingleMarkerTracker final : public IMarkerTracker {
public:
    LegacySingleMarkerTracker();
    ~LegacySingleMarkerTracker() override;

    bool start(const std::vector<std::string>& markerImages, const Frame& firstFrame) override;
    void submit(const Frame& frame) override;
    std::vector<MarkerObservation> latest() const override;
    void stop() override;
    bool drivesPose() const override { return true; }
    std::string describe() const override { return "legacy single-marker tracker (2012)"; }

private:
    std::unique_ptr<legacy_tracking::SingleMarkerTracker> tracker_;
    std::string markerName_;
};

// Adapter over MultiMarkerDetector: synchronized BRISK acquisition and
// batched optical-flow tracking for several markers.
class MultiMarkerTracker final : public IMarkerTracker {
public:
    MultiMarkerTracker();
    ~MultiMarkerTracker() override;

    bool start(const std::vector<std::string>& markerImages, const Frame& firstFrame) override;
    void submit(const Frame& frame) override;
    std::vector<MarkerObservation> latest() const override;
    void stop() override;
    bool drivesPose() const override { return false; }
    std::string describe() const override { return "synchronized multi-marker tracker"; }

private:
    std::unique_ptr<MultiMarkerDetector> detector_;
    std::uint64_t lastSequence_ = 0;
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_TRACKING_MARKER_TRACKERS_H
