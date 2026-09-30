#include "adapters/tracking/MarkerTrackers.h"

#include "legacy_marker_tracker.h"
#include "multi_marker_detector.h"

#include <cstdio>

namespace baekar {
namespace {

std::string baseName(const std::string& path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

}  // namespace

// ---------------------------------------------------------------- legacy

LegacySingleMarkerTracker::LegacySingleMarkerTracker()
    : tracker_(std::make_unique<legacy_tracking::SingleMarkerTracker>()) {}

LegacySingleMarkerTracker::~LegacySingleMarkerTracker() { stop(); }

bool LegacySingleMarkerTracker::start(const std::vector<std::string>& markerImages,
                                      const Frame& firstFrame) {
    if (markerImages.size() != 1) {
        std::fprintf(stderr, "BaekAR: the legacy tracker follows exactly one marker.\n");
        return false;
    }
    markerName_ = baseName(markerImages.front());
    return tracker_->Start(markerImages.front(), firstFrame.bgr);
}

void LegacySingleMarkerTracker::submit(const Frame& frame) {
    if (!frame.placeholder && !frame.bgr.empty()) tracker_->Submit(frame.bgr, frame.sequence);
}

std::vector<MarkerObservation> LegacySingleMarkerTracker::latest() const {
    const legacy_tracking::Snapshot snapshot = tracker_->Latest();
    MarkerObservation observation;
    observation.markerIndex = 0;
    observation.name = markerName_;
    // The 2012 main loop drew and posed the marker when either thread had it.
    observation.found = snapshot.detected || snapshot.tracking;
    observation.tracking = snapshot.tracking;
    observation.poseCorners = snapshot.poseCorners;
    observation.detectionCorners = snapshot.detectionCorners;
    observation.inliers = snapshot.inliers;
    observation.frameSequence = snapshot.frameSequence;
    return {observation};
}

void LegacySingleMarkerTracker::stop() { tracker_->Stop(); }

// ----------------------------------------------------------------- multi

MultiMarkerTracker::MultiMarkerTracker() : detector_(std::make_unique<MultiMarkerDetector>()) {}

MultiMarkerTracker::~MultiMarkerTracker() { stop(); }

bool MultiMarkerTracker::start(const std::vector<std::string>& markerImages, const Frame& firstFrame) {
    {
        std::lock_guard<std::mutex> lock(sequenceMutex_);
        submittedSequences_.clear();
        detectorSequence_ = 0;
    }
    lastSequence_ = 0;
    if (!detector_->Initialize(markerImages)) {
        std::fprintf(stderr, "BaekAR: multi-marker detector initialization failed.\n");
        return false;
    }
    submit(firstFrame);
    std::fprintf(stderr, "BaekAR: synchronized tracker ready for %zu marker(s).\n",
                 detector_->WorkerCount());
    return true;
}

void MultiMarkerTracker::submit(const Frame& frame) {
    if (frame.placeholder || frame.bgr.empty() || frame.sequence == lastSequence_) return;
    {
        std::lock_guard<std::mutex> lock(sequenceMutex_);
        submittedSequences_.emplace_back(++detectorSequence_, frame.sequence);
        if (submittedSequences_.size() > 256) submittedSequences_.pop_front();
    }
    detector_->SubmitFrame(frame.bgr);
    lastSequence_ = frame.sequence;
}

std::uint64_t MultiMarkerTracker::frameSequenceFor(std::uint64_t detectorSequence) const {
    std::lock_guard<std::mutex> lock(sequenceMutex_);
    for (auto it = submittedSequences_.rbegin(); it != submittedSequences_.rend(); ++it)
        if (it->first == detectorSequence) return it->second;
    return 0;
}

std::vector<MarkerObservation> MultiMarkerTracker::latest() const {
    std::vector<MarkerObservation> observations;
    for (const MultiMarkerDetection& detection : detector_->LatestDetections()) {
        MarkerObservation observation;
        observation.markerIndex = detection.markerIndex;
        observation.name = detection.markerName;
        observation.found = detection.found && detection.corners.size() == 4;
        observation.tracking = detection.usingOpticalFlow;
        if (detection.corners.size() == 4) {
            for (int i = 0; i < 4; ++i) {
                observation.poseCorners[i] = detection.corners[i];
                observation.detectionCorners[i] = detection.corners[i];
            }
        }
        observation.inliers = detection.inlierCount;
        observation.trackedPoints = detection.trackedPointCount;
        observation.frameSequence = frameSequenceFor(detection.frameSequence);
        observations.push_back(observation);
    }
    return observations;
}

void MultiMarkerTracker::stop() { detector_->Stop(); }

}  // namespace baekar
