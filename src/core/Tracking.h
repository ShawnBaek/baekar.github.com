#ifndef BAEKAR_CORE_TRACKING_H
#define BAEKAR_CORE_TRACKING_H

#include <opencv2/core.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace baekar {

// One marker in one frame, as published by an IMarkerTracker.
struct MarkerObservation {
    std::size_t markerIndex = 0;
    std::string name;
    bool found = false;
    bool tracking = false;  // frame-to-frame tracking vs. full detection
    // Corners the pose estimator should use (tracking corners while
    // tracking, detection corners otherwise), clockwise from top-left.
    std::array<cv::Point2f, 4> poseCorners{};
    // Last feature-matching (BRISK + homography) result; drawn as the
    // marker outline.
    std::array<cv::Point2f, 4> detectionCorners{};
    int inliers = 0;
    int trackedPoints = 0;
    std::uint64_t frameSequence = 0;
};

// 4x4 matrix as 16 floats. D3DX row-major/row-vector and OpenGL
// column-major/column-vector matrices share this memory layout, so the
// 2012 camera's matrices are stored here unchanged.
struct Mat4 {
    std::array<float, 16> m{};
    static Mat4 identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }
};

struct Pose {
    bool valid = false;
    Mat4 view;
};

// Everything the renderer needs from tracking for one frame.
struct TrackingResult {
    std::vector<MarkerObservation> markers;
    bool drivesPose = false;  // single-marker pipeline: 3D overlay on markers[0]
    Mat4 projection;
    Pose pose;
};

}  // namespace baekar

#endif  // BAEKAR_CORE_TRACKING_H
