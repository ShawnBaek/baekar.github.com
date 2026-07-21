#ifndef BAEKAR_MULTI_MARKER_DETECTOR_H
#define BAEKAR_MULTI_MARKER_DETECTOR_H

#include <opencv2/core.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct MultiMarkerDetection {
    std::size_t markerIndex = 0;
    std::string markerName;
    std::vector<cv::Point2f> corners;
    int candidateCount = 0;
    int inlierCount = 0;
    int trackedPointCount = 0;
    std::uint64_t frameSequence = 0;
    bool usingOpticalFlow = false;
    bool found = false;
};

// Runs one persistent, isolated BRISK matching worker for each marker.
class MultiMarkerDetector {
public:
    MultiMarkerDetector();
    ~MultiMarkerDetector();

    bool Initialize(const std::vector<std::string>& markerPaths);
    void SubmitFrame(const cv::Mat& bgrFrame);
    std::vector<MultiMarkerDetection> LatestDetections() const;
    std::size_t WorkerCount() const;
    void Stop();

    MultiMarkerDetector(const MultiMarkerDetector&) = delete;
    MultiMarkerDetector& operator=(const MultiMarkerDetector&) = delete;

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

#endif  // BAEKAR_MULTI_MARKER_DETECTOR_H
