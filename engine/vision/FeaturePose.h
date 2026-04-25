#pragma once

#include <array>
#include <opencv2/core.hpp>
#include <string>

namespace baekar {

struct CameraIntrinsics {
    cv::Matx33d K;
    cv::Vec4d   dist;
    cv::Size    size;
};

// Loads "fx fy cx cy\nk1 k2 p1 p2" plain-text calibration (the historical
// BaekAR layout — see MarkerlessAR/calibration/calibration.txt).
bool loadCalibration(const std::string& path, CameraIntrinsics& out,
                     cv::Size frameSize = cv::Size(640, 480));

// Solves the marker plane pose. The four image points are the projected
// marker corners in the live frame; the model is the marker placed on Z=0
// with the given physical width/height (in the same world units the GL
// matrices will use; we use millimetres to mirror the Windows demo).
bool solvePose(const CameraIntrinsics& cam,
               const std::array<cv::Point2f, 4>& imagePts,
               float markerWidth, float markerHeight,
               cv::Vec3d& rvec, cv::Vec3d& tvec);

// Build column-major 4x4s suitable for Metal / OpenGL.
void makeProjectionMatrix(const CameraIntrinsics& cam,
                          float nearZ, float farZ,
                          float outColMajor[16]);

void makeViewMatrix(const cv::Vec3d& rvec, const cv::Vec3d& tvec,
                    float outColMajor[16]);

} // namespace baekar
