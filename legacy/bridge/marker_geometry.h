#ifndef BAEKAR_LEGACY_MARKER_GEOMETRY_H
#define BAEKAR_LEGACY_MARKER_GEOMETRY_H

// The three helpers the 2012 single-marker tracker used from
// SungwookFeature.cpp, rewritten on the OpenCV C++ API. SungwookFeature.cpp
// carries BRISK's GPL-3.0 header and pulls in the bundled BRISK, so it is no
// longer built (docs/roadmap.md, license policy). Results are identical;
// tests/characterization/marker_geometry_test.cpp checks the behavior.

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>

#include <vector>

namespace legacy_tracking {

// Flattens k-NN matches into (train point, query point) pairs.
void MatchesToPoints(const std::vector<std::vector<cv::DMatch>>& matches,
                     const std::vector<cv::KeyPoint>& trainKeypoints,
                     const std::vector<cv::KeyPoint>& queryKeypoints,
                     std::vector<cv::Point2f>& trainPoints,
                     std::vector<cv::Point2f>& queryPoints);

// Places a grayscale marker image in the middle of a square canvas whose
// side is the image diagonal, scaled by `scale` about the canvas center
// (the 2012 swMoveImage).
cv::Mat CenterMarkerImage(const cv::Mat& gray, double scale);

// Maps marker-image points into CenterMarkerImage's canvas for scale 0.5
// (the 2012 swMoveCorners, which always used 0.5).
std::vector<cv::Point2f> CenterMarkerCorners(const cv::Size& imageSize,
                                             const std::vector<cv::Point2f>& points);

}  // namespace legacy_tracking

#endif  // BAEKAR_LEGACY_MARKER_GEOMETRY_H
