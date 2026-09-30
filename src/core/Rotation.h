#ifndef BAEKAR_CORE_ROTATION_H
#define BAEKAR_CORE_ROTATION_H

#include <opencv2/core.hpp>

#include <cmath>

namespace baekar {

// Unit quaternion (x, y, z, w), Hamilton convention -> rotation matrix.
inline cv::Matx33d quaternionToRotation(double qx, double qy, double qz, double qw) {
    const double n = std::sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
    if (n <= 0.0) return cv::Matx33d::eye();
    qx /= n; qy /= n; qz /= n; qw /= n;
    return cv::Matx33d(1 - 2 * (qy * qy + qz * qz), 2 * (qx * qy - qz * qw), 2 * (qx * qz + qy * qw),
                       2 * (qx * qy + qz * qw), 1 - 2 * (qx * qx + qz * qz), 2 * (qy * qz - qx * qw),
                       2 * (qx * qz - qy * qw), 2 * (qy * qz + qx * qw), 1 - 2 * (qx * qx + qy * qy));
}

// Rotation matrix -> unit quaternion (x, y, z, w), Shepperd's method.
inline cv::Vec4d rotationToQuaternion(const cv::Matx33d& R) {
    const double trace = R(0, 0) + R(1, 1) + R(2, 2);
    double qx, qy, qz, qw;
    if (trace > 0) {
        const double s = std::sqrt(trace + 1.0) * 2;
        qw = 0.25 * s;
        qx = (R(2, 1) - R(1, 2)) / s;
        qy = (R(0, 2) - R(2, 0)) / s;
        qz = (R(1, 0) - R(0, 1)) / s;
    } else if (R(0, 0) > R(1, 1) && R(0, 0) > R(2, 2)) {
        const double s = std::sqrt(1.0 + R(0, 0) - R(1, 1) - R(2, 2)) * 2;
        qw = (R(2, 1) - R(1, 2)) / s;
        qx = 0.25 * s;
        qy = (R(0, 1) + R(1, 0)) / s;
        qz = (R(0, 2) + R(2, 0)) / s;
    } else if (R(1, 1) > R(2, 2)) {
        const double s = std::sqrt(1.0 + R(1, 1) - R(0, 0) - R(2, 2)) * 2;
        qw = (R(0, 2) - R(2, 0)) / s;
        qx = (R(0, 1) + R(1, 0)) / s;
        qy = 0.25 * s;
        qz = (R(1, 2) + R(2, 1)) / s;
    } else {
        const double s = std::sqrt(1.0 + R(2, 2) - R(0, 0) - R(1, 1)) * 2;
        qw = (R(1, 0) - R(0, 1)) / s;
        qx = (R(0, 2) + R(2, 0)) / s;
        qy = (R(1, 2) + R(2, 1)) / s;
        qz = 0.25 * s;
    }
    return cv::Vec4d(qx, qy, qz, qw);
}

}  // namespace baekar

#endif  // BAEKAR_CORE_ROTATION_H
