// marker_geometry replaced the 2012 helpers in SungwookFeature.cpp (GPL-3.0
// header). The expected values were recorded from the original swMoveImage
// and swMoveCorners on yejin.jpg before that file left the build.

#include "marker_geometry.h"

#include <opencv2/imgcodecs.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

}  // namespace

int main() {
    const cv::Mat gray = cv::imread(std::string(BAEKAR_SOURCE_DIR) + "/assets/image/yejin.jpg",
                                    cv::IMREAD_GRAYSCALE);
    expect(gray.cols == 241 && gray.rows == 318, "yejin.jpg is 241x318");

    const cv::Mat canvas = legacy_tracking::CenterMarkerImage(gray, 0.5);
    expect(canvas.cols == 399 && canvas.rows == 399, "canvas is 399x399");
    expect(cv::sum(canvas)[0] == 1642304.0, "canvas pixel sum matches the 2012 output");
    expect(cv::countNonZero(canvas) == 18204, "canvas non-zero count matches the 2012 output");

    const std::vector<cv::Point2f> corners = {
        {0, 0}, {241, 0}, {241, 318}, {0, 318}};
    const std::vector<cv::Point2f> moved = legacy_tracking::CenterMarkerCorners(gray.size(), corners);
    const float expected[4][2] = {{139.252502f, 120.002502f},
                                  {259.752502f, 120.002502f},
                                  {259.752502f, 279.002502f},
                                  {139.252502f, 279.002502f}};
    for (int i = 0; i < 4; ++i) {
        expect(std::abs(moved[i].x - expected[i][0]) < 1e-4f &&
                   std::abs(moved[i].y - expected[i][1]) < 1e-4f,
               "corner matches the 2012 output");
    }

    std::vector<cv::KeyPoint> train = {cv::KeyPoint(1, 2, 3), cv::KeyPoint(4, 5, 3)};
    std::vector<cv::KeyPoint> query = {cv::KeyPoint(10, 20, 3), cv::KeyPoint(40, 50, 3)};
    std::vector<std::vector<cv::DMatch>> matches = {{cv::DMatch(1, 0, 1.0f)}, {cv::DMatch(0, 1, 1.0f)}};
    std::vector<cv::Point2f> trainPoints, queryPoints;
    legacy_tracking::MatchesToPoints(matches, train, query, trainPoints, queryPoints);
    expect(trainPoints.size() == 2 && trainPoints[0] == cv::Point2f(1, 2) &&
               queryPoints[0] == cv::Point2f(40, 50),
           "matches become (train, query) point pairs");

    if (failures == 0) std::fprintf(stderr, "marker_geometry_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
