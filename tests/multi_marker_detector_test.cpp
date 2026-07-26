#include "multi_marker_detector.h"
#include "synthetic_marker_source.h"

#include <opencv2/core.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace {

bool CornersArePlausible(const std::vector<cv::Point2f>& corners)
{
    if (corners.size() != 4) {
        return false;
    }
    for (const cv::Point2f& corner : corners) {
        if (corner.x < -640.0f || corner.x > 1280.0f ||
            corner.y < -480.0f || corner.y > 960.0f) {
            return false;
        }
    }
    return true;
}

}  // namespace

int main()
{
    const std::vector<std::string> markerNames = {
        "yejin.jpg", "fish.jpg", "cola.jpg", "grafi.jpg", "suji.jpg",
        "hyojoo.jpg", "iu1.jpg", "mina1.jpg", "minjung1.jpg", "minjung4.jpg"
    };
    std::vector<std::string> markerPaths;
    for (const std::string& markerName : markerNames) {
        markerPaths.push_back(std::string(BAEKAR_SOURCE_DIR) +
                              "/MarkerlessAR/image/" + markerName);
    }

    {
        MultiMarkerDetector ambiguousDetector;
        const std::vector<std::string> ambiguousMarkerPaths = {
            std::string(BAEKAR_SOURCE_DIR) + "/MarkerlessAR/image/iu1.jpg",
            std::string(BAEKAR_SOURCE_DIR) + "/MarkerlessAR/image/iumarker.jpg"
        };
        if (ambiguousDetector.Initialize(ambiguousMarkerPaths)) {
            std::fprintf(stderr, "Visually similar markers were not rejected.\n");
            return 1;
        }
    }

    SyntheticMarkerSource source;
    MultiMarkerDetector detector;
    if (!source.Initialize(markerPaths) || !detector.Initialize(markerPaths)) {
        std::fprintf(stderr, "Could not initialize ten-marker tracking test.\n");
        return 1;
    }

    int observedSnapshots = 0;
    int allFoundSnapshots = 0;
    std::uint64_t lastObservedSequence = 0;
    for (int frameIndex = 0; frameIndex < 75; ++frameIndex) {
        cv::Mat frame;
        if (!source.NextFrame(frame)) {
            std::fprintf(stderr, "Synthetic frame %d failed.\n", frameIndex);
            return 1;
        }
        detector.SubmitFrame(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(12));

        const std::vector<MultiMarkerDetection> detections =
            detector.LatestDetections();
        if (detections.size() != markerPaths.size()) {
            std::fprintf(stderr, "Expected %zu detections, received %zu.\n",
                         markerPaths.size(), detections.size());
            return 1;
        }
        if (detections.empty() || detections.front().frameSequence == 0 ||
            detections.front().frameSequence == lastObservedSequence) {
            continue;
        }

        const std::uint64_t snapshotSequence = detections.front().frameSequence;
        bool allFound = true;
        for (const MultiMarkerDetection& detection : detections) {
            if (detection.frameSequence != snapshotSequence) {
                std::fprintf(stderr,
                             "Marker snapshot was not published atomically: %llu vs %llu.\n",
                             static_cast<unsigned long long>(snapshotSequence),
                             static_cast<unsigned long long>(detection.frameSequence));
                return 1;
            }
            if (!detection.found || !CornersArePlausible(detection.corners)) {
                allFound = false;
            }
        }

        lastObservedSequence = snapshotSequence;
        ++observedSnapshots;
        if (allFound) {
            ++allFoundSnapshots;
        }
    }

    cv::Mat occludedFrame(480, 640, CV_8UC3, cv::Scalar(24, 24, 24));
    for (int frameIndex = 0; frameIndex < 12; ++frameIndex) {
        detector.SubmitFrame(occludedFrame);
        std::this_thread::sleep_for(std::chrono::milliseconds(12));
    }
    const std::vector<MultiMarkerDetection> occludedDetections =
        detector.LatestDetections();
    bool observedLostMarker = false;
    for (const MultiMarkerDetection& detection : occludedDetections) {
        observedLostMarker = observedLostMarker || !detection.found;
    }
    if (!observedLostMarker) {
        std::fprintf(stderr, "Occluded markers were still reported as found.\n");
        return 1;
    }

    bool recoveredAllMarkers = false;
    for (int frameIndex = 0; frameIndex < 30; ++frameIndex) {
        cv::Mat frame;
        source.NextFrame(frame);
        detector.SubmitFrame(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(12));

        const std::vector<MultiMarkerDetection> detections =
            detector.LatestDetections();
        if (detections.empty() || detections.front().frameSequence == 0) {
            continue;
        }
        const std::uint64_t snapshotSequence = detections.front().frameSequence;
        bool allFound = true;
        for (const MultiMarkerDetection& detection : detections) {
            allFound = allFound && detection.frameSequence == snapshotSequence &&
                detection.found && CornersArePlausible(detection.corners);
        }
        recoveredAllMarkers = recoveredAllMarkers || allFound;
    }
    detector.Stop();

    if (observedSnapshots < 12) {
        std::fprintf(stderr, "Only %d synchronized snapshots were processed.\n",
                     observedSnapshots);
        return 1;
    }
    if (allFoundSnapshots < observedSnapshots * 9 / 10) {
        std::fprintf(stderr,
                     "All markers were found in only %d of %d snapshots.\n",
                     allFoundSnapshots, observedSnapshots);
        return 1;
    }
    if (!recoveredAllMarkers) {
        std::fprintf(stderr, "All markers did not recover after occlusion.\n");
        return 1;
    }

    std::fprintf(stderr,
                 "BaekAR tracking test passed: %d/%d synchronized snapshots found all markers; occlusion recovery passed.\n",
                 allFoundSnapshots, observedSnapshots);
    return 0;
}
