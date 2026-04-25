#include "engine/vision/FeaturePose.h"

#include <opencv2/calib3d.hpp>
#include <fstream>

namespace baekar {

bool loadCalibration(const std::string& path, CameraIntrinsics& out,
                     cv::Size frameSize) {
    std::ifstream f(path);
    if (!f) return false;
    double fx, fy, cx, cy, k1, k2, p1, p2;
    if (!(f >> fx >> fy >> cx >> cy)) return false;
    if (!(f >> k1 >> k2 >> p1 >> p2)) return false;
    out.K = cv::Matx33d(fx, 0,  cx,
                        0,  fy, cy,
                        0,  0,  1);
    out.dist = cv::Vec4d(k1, k2, p1, p2);
    out.size = frameSize;
    return true;
}

bool solvePose(const CameraIntrinsics& cam,
               const std::array<cv::Point2f, 4>& imagePts,
               float markerWidth, float markerHeight,
               cv::Vec3d& rvec, cv::Vec3d& tvec) {
    // Marker corners on the Z=0 plane, origin at the centre to match the
    // historical convention used by `swMoveCorners` in SungwookFeature.cpp.
    const float w = markerWidth  * 0.5f;
    const float h = markerHeight * 0.5f;
    std::vector<cv::Point3f> obj = {
        { -w, -h, 0.0f },
        {  w, -h, 0.0f },
        {  w,  h, 0.0f },
        { -w,  h, 0.0f },
    };
    std::vector<cv::Point2f> img(imagePts.begin(), imagePts.end());
    return cv::solvePnP(obj, img, cv::Mat(cam.K), cv::Mat(cam.dist),
                        rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
}

void makeProjectionMatrix(const CameraIntrinsics& cam,
                          float nearZ, float farZ,
                          float out[16]) {
    // Standard pinhole-to-GL projection. Result is column-major.
    const float fx = static_cast<float>(cam.K(0, 0));
    const float fy = static_cast<float>(cam.K(1, 1));
    const float cx = static_cast<float>(cam.K(0, 2));
    const float cy = static_cast<float>(cam.K(1, 2));
    const float W  = static_cast<float>(cam.size.width);
    const float H  = static_cast<float>(cam.size.height);

    for (int i = 0; i < 16; ++i) out[i] = 0.0f;
    out[0]  =  2.0f * fx / W;
    out[5]  = -2.0f * fy / H;          // flip Y for clip space
    out[8]  = 1.0f - 2.0f * cx / W;
    out[9]  = 2.0f * cy / H - 1.0f;
    out[10] = -(farZ + nearZ) / (farZ - nearZ);
    out[11] = -1.0f;
    out[14] = -2.0f * farZ * nearZ / (farZ - nearZ);
}

void makeViewMatrix(const cv::Vec3d& rvec, const cv::Vec3d& tvec,
                    float out[16]) {
    cv::Mat R;
    cv::Rodrigues(rvec, R);
    // OpenCV's solvePnP returns a right-handed camera looking down +Z.
    // GL/Metal cameras look down -Z, so flip Y and Z rows.
    double mv[16];
    for (int r = 0; r < 3; ++r) {
        const double s = (r == 0) ? 1.0 : -1.0;
        mv[0 + r] = s * R.at<double>(r, 0);
        mv[4 + r] = s * R.at<double>(r, 1);
        mv[8 + r] = s * R.at<double>(r, 2);
        mv[12 + r] = s * tvec[r];
    }
    mv[3]  = 0.0;
    mv[7]  = 0.0;
    mv[11] = 0.0;
    mv[15] = 1.0;
    for (int i = 0; i < 16; ++i) out[i] = static_cast<float>(mv[i]);
}

} // namespace baekar
