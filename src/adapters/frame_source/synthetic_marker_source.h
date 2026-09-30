#ifndef BAEKAR_SYNTHETIC_MARKER_SOURCE_H
#define BAEKAR_SYNTHETIC_MARKER_SOURCE_H

#include <opencv2/core.hpp>

#include <string>
#include <vector>

// Ground truth for one marker in the last generated frame.
struct SyntheticMarkerTruth {
    // Image outline of the marker picture, clockwise from its top-left
    // pixel (0,0), (w-1,0), (w-1,h-1), (0,h-1).
    std::vector<cv::Point2f> corners;
    cv::Vec3d rvec;  // marker pose in the camera frame (Rodrigues)
    cv::Vec3d tvec;  // metres
};

// A deterministic, camera-free source useful for exercising marker tracking.
class SyntheticMarkerSource {
public:
    SyntheticMarkerSource();

    bool Initialize(const std::string& markerPath,
                    int frameWidth = 640,
                    int frameHeight = 480);
    bool Initialize(const std::vector<std::string>& markerPaths,
                    int frameWidth = 640,
                    int frameHeight = 480);
    bool NextFrame(cv::Mat& bgrFrame);
    bool IsInitialized() const;
    // Truth for the frame returned by the last NextFrame call.
    const std::vector<SyntheticMarkerTruth>& LastTruth() const { return lastTruth_; }
    // Pinhole camera used to render the frames (no distortion).
    cv::Matx33d CameraMatrix() const;

private:
    std::vector<cv::Mat> markerBgrs_;
    std::vector<SyntheticMarkerTruth> lastTruth_;
    cv::Mat background_;
    int frameWidth_;
    int frameHeight_;
    unsigned long frameIndex_;
    bool initialized_;
};

#endif  // BAEKAR_SYNTHETIC_MARKER_SOURCE_H
