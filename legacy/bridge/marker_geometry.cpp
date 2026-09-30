#include "marker_geometry.h"

#include <opencv2/imgproc.hpp>

#include <cmath>

namespace legacy_tracking {
namespace {

// Half the diagonal of the canvas. The 2012 code computed it for `scale`
// and then, for scale <= 1, for the unscaled image, so only the unscaled
// case ever mattered for scale <= 1.
double canvasRadius(const cv::Size& size, double scale) {
    const double s = scale <= 1.0 ? 1.0 : scale;
    const double x = size.width * s / 2.0;
    const double y = size.height * s / 2.0;
    return std::sqrt(x * x + y * y);
}

}  // namespace

void MatchesToPoints(const std::vector<std::vector<cv::DMatch>>& matches,
                     const std::vector<cv::KeyPoint>& trainKeypoints,
                     const std::vector<cv::KeyPoint>& queryKeypoints,
                     std::vector<cv::Point2f>& trainPoints,
                     std::vector<cv::Point2f>& queryPoints) {
    trainPoints.clear();
    queryPoints.clear();
    for (const std::vector<cv::DMatch>& candidates : matches) {
        for (const cv::DMatch& match : candidates) {
            queryPoints.push_back(queryKeypoints[match.queryIdx].pt);
            trainPoints.push_back(trainKeypoints[match.trainIdx].pt);
        }
    }
}

cv::Mat CenterMarkerImage(const cv::Mat& gray, double scale) {
    const double r = canvasRadius(gray.size(), scale);
    const int side = static_cast<int>(r * 2);  // truncation, as cvSize(double) did

    cv::Mat canvas = cv::Mat::zeros(side, side, CV_8UC1);
    const cv::Rect roi(canvas.cols / 2 - gray.cols / 2, canvas.rows / 2 - gray.rows / 2, gray.cols,
                       gray.rows);
    gray.copyTo(canvas(roi));

    const cv::Point2f center(canvas.cols / 2.0f, canvas.rows / 2.0f);
    const cv::Mat rotation = cv::getRotationMatrix2D(center, 0.0, scale);
    // cvWarpAffine without CV_WARP_FILL_OUTLIERS left pixels that map
    // outside the source untouched in the zeroed destination.
    cv::Mat result = cv::Mat::zeros(side, side, CV_8UC1);
    cv::warpAffine(canvas, result, rotation, result.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    return result;
}

std::vector<cv::Point2f> CenterMarkerCorners(const cv::Size& imageSize,
                                             const std::vector<cv::Point2f>& points) {
    const double scale = 0.5;
    const double r = canvasRadius(imageSize, scale);
    const cv::Mat m = cv::getRotationMatrix2D(
        cv::Point2f(imageSize.width / 2.0f, imageSize.height / 2.0f), 0.0, scale);

    std::vector<cv::Point2f> out(points.size());
    for (std::size_t i = 0; i < points.size(); ++i) {
        const float x = static_cast<float>(m.at<double>(0, 0) * points[i].x +
                                           m.at<double>(0, 1) * points[i].y + m.at<double>(0, 2) +
                                           (r - imageSize.width / 2.0));
        const float y = static_cast<float>(m.at<double>(1, 0) * points[i].x +
                                           m.at<double>(1, 1) * points[i].y + m.at<double>(1, 2) +
                                           (r - imageSize.height / 2.0));
        out[i] = cv::Point2f(x, y);
    }
    return out;
}

}  // namespace legacy_tracking
