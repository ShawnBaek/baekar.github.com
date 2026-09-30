#ifndef BAEKAR_ADAPTERS_TRACKING_MARKER_TRACKERS_H
#define BAEKAR_ADAPTERS_TRACKING_MARKER_TRACKERS_H

#include "application/ports/IMarkerTracker.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <utility>

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
    std::uint64_t frameSequenceFor(std::uint64_t detectorSequence) const;

    std::unique_ptr<MultiMarkerDetector> detector_;
    std::uint64_t lastSequence_ = 0;
    // The detector numbers submitted frames 1, 2, 3, ...; this maps those
    // numbers back to Frame::sequence for recent frames.
    mutable std::mutex sequenceMutex_;
    std::deque<std::pair<std::uint64_t, std::uint64_t>> submittedSequences_;
    std::uint64_t detectorSequence_ = 0;
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_TRACKING_MARKER_TRACKERS_H
