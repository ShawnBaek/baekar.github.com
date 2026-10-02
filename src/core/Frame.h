#ifndef BAEKAR_CORE_FRAME_H
#define BAEKAR_CORE_FRAME_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace baekar {

// The engine processes 640x480 BGR frames (the 2012 pipeline's size).
constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;

// Pinhole intrinsics with OpenCV's (k1, k2, p1, p2, k3) distortion.
struct CameraIntrinsics {
    double fx = 0.0, fy = 0.0, cx = 0.0, cy = 0.0;
    std::array<double, 5> distortion{};
    int width = 0, height = 0;

    cv::Matx33d matrix() const { return cv::Matx33d(fx, 0, cx, 0, fy, cy, 0, 0, 1); }
    // Intrinsics for the same camera at another image size.
    CameraIntrinsics scaledTo(int newWidth, int newHeight) const {
        CameraIntrinsics s = *this;
        const double sx = static_cast<double>(newWidth) / width;
        const double sy = static_cast<double>(newHeight) / height;
        s.fx *= sx; s.cx *= sx; s.fy *= sy; s.cy *= sy;
        s.width = newWidth; s.height = newHeight;
        return s;
    }
};

// Rigid transform. For a camera pose it maps camera coordinates to world
// coordinates (world_from_camera): p_world = R * p_camera + t.
struct RigidTransform {
    cv::Matx33d R = cv::Matx33d::eye();
    cv::Vec3d t{0, 0, 0};

    RigidTransform inverse() const { return {R.t(), -(R.t() * t)}; }
    RigidTransform operator*(const RigidTransform& o) const { return {R * o.R, R * o.t + t}; }
};

struct ImuSample {
    double timestampSeconds = 0.0;
    cv::Vec3d gyro;   // rad/s, sensor frame
    cv::Vec3d accel;  // m/s^2, sensor frame
};

// One camera image plus whatever else the source knows about it. Sources
// fill only what they have; optional parts stay empty. A source may hand
// out the same image twice (for example when AVFoundation has no fresh
// frame yet); the sequence number only increases for a new image.
struct Frame {
    cv::Mat bgr;                 // CV_8UC3
    std::uint64_t sequence = 0;  // 0 means "no frame yet"
    std::int64_t tickCount = 0;  // cv::getTickCount() when captured
    bool placeholder = false;    // true for "camera unavailable" frames

    double timestampSeconds = 0.0;             // sensor time, if known
    cv::Mat depth;                             // CV_32F metres, 0 = no depth
    std::optional<CameraIntrinsics> intrinsics;
    std::vector<ImuSample> imu;                // samples since the previous frame
    std::optional<RigidTransform> referencePose;  // world_from_camera ground truth
    // Ground-truth outline of each marker (clockwise from the marker
    // image's top-left corner), for synthetic scenes.
    std::vector<std::array<cv::Point2f, 4>> referenceMarkerCorners;
};

// Resizes to the engine frame size with the interpolation the 2012 Capture
// class used; returns the input unchanged if it already fits.
inline cv::Mat toEngineFrameSize(const cv::Mat& bgr) {
    if (bgr.cols == kFrameWidth && bgr.rows == kFrameHeight) return bgr;
    cv::Mat resized;
    cv::resize(bgr, resized, cv::Size(kFrameWidth, kFrameHeight), 0, 0, cv::INTER_AREA);
    return resized;
}

}  // namespace baekar

#endif  // BAEKAR_CORE_FRAME_H
