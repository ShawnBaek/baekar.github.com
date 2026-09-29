#include "synthetic_marker_source.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <cmath>
#include <vector>

namespace {

const double kPi = 3.14159265358979323846;

cv::Mat MakeBackground(int width, int height, std::size_t markerCount)
{
    cv::Mat background(height, width, CV_8UC3, cv::Scalar(207, 207, 207));

    const cv::Scalar gridColor(194, 194, 194);
    for (int x = 0; x < width; x += 40) {
        cv::line(background, cv::Point(x, 0), cv::Point(x, height - 1), gridColor, 1);
    }
    for (int y = 0; y < height; y += 40) {
        cv::line(background, cv::Point(0, y), cv::Point(width - 1, y), gridColor, 1);
    }

    const char* title = markerCount > 1
        ? "BaekAR multi-marker simulation"
        : "BaekAR marker simulation";
    cv::putText(background, title, cv::Point(12, height - 14),
                cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(82, 82, 82), 1, cv::LINE_AA);
    return background;
}

cv::Mat RotationVector(double roll, double pitch, double yaw)
{
    const cv::Matx33d rx(1.0, 0.0, 0.0,
                         0.0, std::cos(pitch), -std::sin(pitch),
                         0.0, std::sin(pitch), std::cos(pitch));
    const cv::Matx33d ry(std::cos(yaw), 0.0, std::sin(yaw),
                         0.0, 1.0, 0.0,
                         -std::sin(yaw), 0.0, std::cos(yaw));
    const cv::Matx33d rz(std::cos(roll), -std::sin(roll), 0.0,
                         std::sin(roll), std::cos(roll), 0.0,
                         0.0, 0.0, 1.0);
    cv::Mat rotation = cv::Mat(rz * ry * rx);
    cv::Mat rvec;
    cv::Rodrigues(rotation, rvec);
    return rvec;
}

}  // namespace

SyntheticMarkerSource::SyntheticMarkerSource()
    : frameWidth_(0), frameHeight_(0), frameIndex_(0), initialized_(false)
{
}

bool SyntheticMarkerSource::Initialize(const std::string& markerPath,
                                       int frameWidth,
                                       int frameHeight)
{
    return Initialize(std::vector<std::string>(1, markerPath),
                      frameWidth, frameHeight);
}

bool SyntheticMarkerSource::Initialize(const std::vector<std::string>& markerPaths,
                                       int frameWidth,
                                       int frameHeight)
{
    initialized_ = false;
    markerBgrs_.clear();
    if (markerPaths.empty() || frameWidth <= 0 || frameHeight <= 0) {
        return false;
    }

    for (const std::string& markerPath : markerPaths) {
        cv::Mat marker = cv::imread(markerPath, cv::IMREAD_COLOR);
        if (marker.empty()) {
            markerBgrs_.clear();
            return false;
        }
        markerBgrs_.push_back(marker);
    }

    frameWidth_ = frameWidth;
    frameHeight_ = frameHeight;
    background_ = MakeBackground(frameWidth_, frameHeight_, markerBgrs_.size());
    frameIndex_ = 0;
    initialized_ = true;
    return true;
}

bool SyntheticMarkerSource::NextFrame(cv::Mat& bgrFrame)
{
    if (!initialized_) {
        bgrFrame.release();
        return false;
    }

    const double time = static_cast<double>(frameIndex_++) * 0.045;
    const double focalLength = 0.90 * frameWidth_;

    const cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        focalLength, 0.0, frameWidth_ * 0.5,
        0.0, focalLength, frameHeight_ * 0.5,
        0.0, 0.0, 1.0);
    bgrFrame = background_.clone();

    const std::size_t markerCount = markerBgrs_.size();
    const int columnCount = markerCount == 1 ? 1 : static_cast<int>(
        std::ceil(std::sqrt(static_cast<double>(markerCount) * 4.0 / 3.0)));
    const int rowCount = static_cast<int>((markerCount + columnCount - 1) / columnCount);
    const double columnSpacing = markerCount == 1 ? 0.0 : 5.20 / columnCount;
    const double rowSpacing = markerCount == 1 ? 0.0 : 4.00 / rowCount;
    const double maximumMarkerHeight = markerCount == 1
        ? 1.70 : std::min(1.45, rowSpacing * 0.78);

    for (std::size_t index = 0; index < markerCount; ++index) {
        const cv::Mat& marker = markerBgrs_[index];
        const double markerAspect = static_cast<double>(marker.cols) / marker.rows;
        const double markerHeight = markerCount == 1
            ? maximumMarkerHeight
            : std::min(maximumMarkerHeight,
                       columnSpacing * 0.88 / markerAspect);
        const double markerWidth = markerHeight * markerAspect;
        const double phase = static_cast<double>(index) * 1.73;
        const int column = static_cast<int>(index) % columnCount;
        const int row = static_cast<int>(index) / columnCount;
        const double baseX = (column - (columnCount - 1) * 0.5) * columnSpacing;
        const double baseY = (row - (rowCount - 1) * 0.5) * rowSpacing;

        const std::vector<cv::Point3f> objectCorners = {
            cv::Point3f(static_cast<float>(-markerWidth * 0.5), static_cast<float>(-markerHeight * 0.5), 0.0f),
            cv::Point3f(static_cast<float>( markerWidth * 0.5), static_cast<float>(-markerHeight * 0.5), 0.0f),
            cv::Point3f(static_cast<float>( markerWidth * 0.5), static_cast<float>( markerHeight * 0.5), 0.0f),
            cv::Point3f(static_cast<float>(-markerWidth * 0.5), static_cast<float>( markerHeight * 0.5), 0.0f)
        };
        const cv::Mat rvec = RotationVector(
            0.34 * std::sin(time * (0.72 + index * 0.03) + phase),
            0.24 * std::sin(time * (0.94 + index * 0.04) + 0.4 + phase),
            0.28 * std::cos(time * (0.61 + index * 0.05) + phase));
        const cv::Mat tvec = (cv::Mat_<double>(3, 1) <<
            baseX + (markerCount == 1 ? 0.52 : 0.14) *
                std::sin(time * 0.63 + phase),
            baseY + (markerCount == 1 ? 0.34 : 0.10) *
                std::sin(time * 0.91 + 1.0 + phase),
            (markerCount == 1 ? 5.10 : 5.65) +
                (markerCount == 1 ? 1.05 : 0.35) * std::sin(time * 0.51 + phase));

        std::vector<cv::Point2f> projectedCorners;
        cv::projectPoints(objectCorners, rvec, tvec, cameraMatrix,
                          cv::Mat(), projectedCorners);

        const std::vector<cv::Point2f> markerCorners = {
            cv::Point2f(0.0f, 0.0f),
            cv::Point2f(static_cast<float>(marker.cols - 1), 0.0f),
            cv::Point2f(static_cast<float>(marker.cols - 1), static_cast<float>(marker.rows - 1)),
            cv::Point2f(0.0f, static_cast<float>(marker.rows - 1))
        };
        const cv::Mat transform = cv::getPerspectiveTransform(markerCorners,
                                                               projectedCorners);

        cv::Mat warpedMarker;
        cv::Mat sourceMask(marker.size(), CV_8UC1, cv::Scalar(255));
        cv::Mat warpedMask;
        cv::warpPerspective(marker, warpedMarker, transform, background_.size(),
                            cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        cv::warpPerspective(sourceMask, warpedMask, transform, background_.size(),
                            cv::INTER_NEAREST, cv::BORDER_CONSTANT);
        warpedMarker.copyTo(bgrFrame, warpedMask);
    }
    return true;
}

bool SyntheticMarkerSource::IsInitialized() const
{
    return initialized_;
}
