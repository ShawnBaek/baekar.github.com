#include "synthetic_marker_source.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <cmath>
#include <vector>

namespace {

const double kPi = 3.14159265358979323846;

cv::Mat MakeBackground(int width, int height)
{
    cv::Mat background(height, width, CV_8UC3, cv::Scalar(207, 207, 207));

    const cv::Scalar gridColor(194, 194, 194);
    for (int x = 0; x < width; x += 40) {
        cv::line(background, cv::Point(x, 0), cv::Point(x, height - 1), gridColor, 1);
    }
    for (int y = 0; y < height; y += 40) {
        cv::line(background, cv::Point(0, y), cv::Point(width - 1, y), gridColor, 1);
    }

    cv::putText(background, "BaekAR marker simulation", cv::Point(12, height - 14),
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
    initialized_ = false;
    markerBgr_ = cv::imread(markerPath, cv::IMREAD_COLOR);
    if (markerBgr_.empty() || frameWidth <= 0 || frameHeight <= 0) {
        return false;
    }

    frameWidth_ = frameWidth;
    frameHeight_ = frameHeight;
    background_ = MakeBackground(frameWidth_, frameHeight_);
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
    const double markerAspect = static_cast<double>(markerBgr_.cols) / markerBgr_.rows;
    const double markerHeight = 1.70;
    const double markerWidth = markerHeight * markerAspect;
    const double focalLength = 0.90 * frameWidth_;

    const cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        focalLength, 0.0, frameWidth_ * 0.5,
        0.0, focalLength, frameHeight_ * 0.5,
        0.0, 0.0, 1.0);
    const std::vector<cv::Point3f> objectCorners = {
        cv::Point3f(static_cast<float>(-markerWidth * 0.5), static_cast<float>(-markerHeight * 0.5), 0.0f),
        cv::Point3f(static_cast<float>( markerWidth * 0.5), static_cast<float>(-markerHeight * 0.5), 0.0f),
        cv::Point3f(static_cast<float>( markerWidth * 0.5), static_cast<float>( markerHeight * 0.5), 0.0f),
        cv::Point3f(static_cast<float>(-markerWidth * 0.5), static_cast<float>( markerHeight * 0.5), 0.0f)
    };
    const cv::Mat rvec = RotationVector(0.55 * std::sin(time * 0.8),
                                        0.34 * std::sin(time * 1.1 + 0.4),
                                        0.43 * std::cos(time * 0.7));
    const cv::Mat tvec = (cv::Mat_<double>(3, 1) <<
        0.52 * std::sin(time * 0.63),
        0.34 * std::sin(time * 0.91 + 1.0),
        5.10 + 1.05 * std::sin(time * 0.51));

    std::vector<cv::Point2f> projectedCorners;
    cv::projectPoints(objectCorners, rvec, tvec, cameraMatrix, cv::Mat(), projectedCorners);

    const std::vector<cv::Point2f> markerCorners = {
        cv::Point2f(0.0f, 0.0f),
        cv::Point2f(static_cast<float>(markerBgr_.cols - 1), 0.0f),
        cv::Point2f(static_cast<float>(markerBgr_.cols - 1), static_cast<float>(markerBgr_.rows - 1)),
        cv::Point2f(0.0f, static_cast<float>(markerBgr_.rows - 1))
    };
    const cv::Mat transform = cv::getPerspectiveTransform(markerCorners, projectedCorners);

    cv::Mat warpedMarker;
    cv::Mat sourceMask(markerBgr_.size(), CV_8UC1, cv::Scalar(255));
    cv::Mat warpedMask;
    cv::warpPerspective(markerBgr_, warpedMarker, transform, background_.size(),
                        cv::INTER_LINEAR, cv::BORDER_CONSTANT);
    cv::warpPerspective(sourceMask, warpedMask, transform, background_.size(),
                        cv::INTER_NEAREST, cv::BORDER_CONSTANT);

    bgrFrame = background_.clone();
    warpedMarker.copyTo(bgrFrame, warpedMask);
    return true;
}

bool SyntheticMarkerSource::IsInitialized() const
{
    return initialized_;
}
