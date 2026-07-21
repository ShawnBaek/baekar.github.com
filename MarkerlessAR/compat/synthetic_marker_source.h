#ifndef BAEKAR_SYNTHETIC_MARKER_SOURCE_H
#define BAEKAR_SYNTHETIC_MARKER_SOURCE_H

#include <opencv2/core.hpp>

#include <string>
#include <vector>

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

private:
    std::vector<cv::Mat> markerBgrs_;
    cv::Mat background_;
    int frameWidth_;
    int frameHeight_;
    unsigned long frameIndex_;
    bool initialized_;
};

#endif  // BAEKAR_SYNTHETIC_MARKER_SOURCE_H
