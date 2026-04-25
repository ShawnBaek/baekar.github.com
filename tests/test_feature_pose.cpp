// Smoke test for engine/vision/FeaturePose. Synthesises a known camera
// pose, projects the four marker corners through it, then asks solvePose
// to recover the pose and asserts the round trip matches.

#include "engine/vision/FeaturePose.h"

#include <opencv2/calib3d.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

void check(bool cond, const char* msg) {
    if (!cond) {
        std::fprintf(stderr, "FAIL: %s\n", msg);
        std::exit(1);
    }
}

} // namespace

int main() {
    baekar::CameraIntrinsics cam;
    cam.K = cv::Matx33d(664.0, 0.0, 320.0,
                        0.0, 673.0, 240.0,
                        0.0, 0.0, 1.0);
    cam.dist = cv::Vec4d(0, 0, 0, 0);
    cam.size = cv::Size(640, 480);

    const float markerSize = 100.0f;
    const float w = markerSize * 0.5f;
    std::vector<cv::Point3f> obj = {
        {-w, -w, 0}, { w, -w, 0}, { w,  w, 0}, {-w,  w, 0}
    };

    cv::Vec3d trueR(0.05, -0.10, 0.02);
    cv::Vec3d trueT(20.0, -10.0, 400.0);

    std::vector<cv::Point2f> img;
    cv::projectPoints(obj, trueR, trueT, cv::Mat(cam.K), cv::Mat(cam.dist), img);
    std::array<cv::Point2f, 4> imgArr{img[0], img[1], img[2], img[3]};

    cv::Vec3d r, t;
    check(baekar::solvePose(cam, imgArr, markerSize, markerSize, r, t),
          "solvePose returned false");

    const double dt = cv::norm(t - trueT);
    const double dr = cv::norm(r - trueR);
    std::printf("|dT| = %.3f mm, |dR| = %.4f rad\n", dt, dr);
    check(dt < 1.0,  "translation error > 1mm");
    check(dr < 1e-3, "rotation error > 1mrad");

    float P[16];
    baekar::makeProjectionMatrix(cam, 1.0f, 1000.0f, P);
    check(std::abs(P[11] + 1.0f) < 1e-6f, "projection w-row wrong");

    float V[16];
    baekar::makeViewMatrix(r, t, V);
    check(std::abs(V[15] - 1.0f) < 1e-6f, "view homogeneous bottom-right wrong");
    check(std::abs(V[3]) < 1e-6f && std::abs(V[7]) < 1e-6f && std::abs(V[11]) < 1e-6f,
          "view top three rows must end in 0");

    std::printf("test_feature_pose: OK\n");
    return 0;
}
