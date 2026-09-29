#ifndef BAEKAR_LEGACY_MARKER_TRACKER_H
#define BAEKAR_LEGACY_MARKER_TRACKER_H

// The 2012 single-marker pipeline (ThreadBRISKMatching + ThreadTracking from
// EngineMain.cpp) as a class. Plain C++ header so C++17 code can use it.

#include <opencv2/core.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace legacy_tracking {

struct Snapshot {
    bool detected = false;  // former bThreadDetection1
    bool tracking = false;  // former bThreadTracking1
    std::array<cv::Point2f, 4> poseCorners{};       // former camera.featuresResult.vertex
    std::array<cv::Point2f, 4> detectionCorners{};  // former dst_matching_corners1
    int inliers = 0;
    std::uint64_t frameSequence = 0;
};

class SingleMarkerTracker {
public:
    SingleMarkerTracker();
    ~SingleMarkerTracker();

    SingleMarkerTracker(const SingleMarkerTracker&) = delete;
    SingleMarkerTracker& operator=(const SingleMarkerTracker&) = delete;

    // Builds the BRISK database from the marker image (former
    // InitializeEngineMain section) and starts both worker threads.
    bool Start(const std::string& markerImagePath, const cv::Mat& firstFrame);
    // Hands a new frame to the workers (former setSharedFrame).
    void Submit(const cv::Mat& bgr, std::uint64_t sequence);
    Snapshot Latest() const;
    // Stops and joins both workers.
    void Stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace legacy_tracking

#endif  // BAEKAR_LEGACY_MARKER_TRACKER_H
