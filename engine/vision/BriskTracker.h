#pragma once

#include <array>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <vector>

namespace baekar {

// Replaces ThreadBRISKMatching + ThreadTracking from the original
// EngineMain.cpp. The class is single-threaded; the host can invoke
// `detectAndMatch` from a worker thread and `track` from another, or
// keep both on the same thread to start.
class BriskTracker {
public:
    BriskTracker();

    // Reference image must be BGR. Stores keypoints + descriptors and
    // the four image corners as the marker boundary.
    bool loadReference(const cv::Mat& refBgr);

    // Returns true and fills `corners` if the four marker corners were
    // located in the live frame via BRISK + RANSAC homography.
    bool detectAndMatch(const cv::Mat& frameBgr,
                        std::array<cv::Point2f, 4>& corners);

    // Refines the four corners frame-to-frame using sparse Lucas-Kanade
    // optical flow seeded from the previous matched feature locations.
    // `prevGray` and `currGray` are CV_8U single-channel images at the
    // capture resolution.
    bool track(const cv::Mat& prevGray, const cv::Mat& currGray,
               std::array<cv::Point2f, 4>& corners);

    // Caller-visible reference data so a debug overlay can draw it.
    const cv::Size& referenceSize() const { return _refSize; }
    const std::vector<cv::Point2f>& matchedRefPts() const { return _refMatched; }
    const std::vector<cv::Point2f>& matchedFramePts() const { return _frameMatched; }

private:
    cv::Ptr<cv::BRISK>   _brisk;
    cv::Ptr<cv::BFMatcher> _matcher;

    cv::Mat              _refDesc;
    std::vector<cv::KeyPoint> _refKp;
    std::array<cv::Point2f, 4> _refCorners;
    cv::Size             _refSize;

    // Last successful match correspondences, also seeded into KLT.
    std::vector<cv::Point2f> _refMatched;
    std::vector<cv::Point2f> _frameMatched;
};

} // namespace baekar
