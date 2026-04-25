#include "engine/vision/BriskTracker.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

namespace baekar {

namespace {
// Reject obviously-degenerate homographies. Mirrors `niceHomography`
// from the legacy SungwookFeature module but in C++17 form.
bool niceHomography(const cv::Mat& H) {
    if (H.empty() || H.rows != 3 || H.cols != 3) return false;
    const double det = cv::determinant(H);
    if (std::abs(det) < 1e-3) return false;
    // Affine sub-block must be reasonably conditioned.
    const double a = H.at<double>(0, 0);
    const double b = H.at<double>(0, 1);
    const double c = H.at<double>(1, 0);
    const double d = H.at<double>(1, 1);
    const double subDet = a * d - b * c;
    if (std::abs(subDet) < 1e-3) return false;
    return true;
}
} // namespace

BriskTracker::BriskTracker()
    : _brisk(cv::BRISK::create(60, 2)),
      _matcher(cv::makePtr<cv::BFMatcher>(cv::NORM_HAMMING, false)) {}

bool BriskTracker::loadReference(const cv::Mat& refBgr) {
    if (refBgr.empty()) return false;
    cv::Mat gray;
    cv::cvtColor(refBgr, gray, cv::COLOR_BGR2GRAY);
    _refKp.clear();
    _refDesc.release();
    _brisk->detectAndCompute(gray, cv::noArray(), _refKp, _refDesc);
    if (_refKp.size() < 8) return false;
    _refSize = gray.size();
    _refCorners = {
        cv::Point2f(0.0f, 0.0f),
        cv::Point2f(static_cast<float>(_refSize.width), 0.0f),
        cv::Point2f(static_cast<float>(_refSize.width),
                    static_cast<float>(_refSize.height)),
        cv::Point2f(0.0f, static_cast<float>(_refSize.height)),
    };
    return true;
}

bool BriskTracker::detectAndMatch(const cv::Mat& frameBgr,
                                  std::array<cv::Point2f, 4>& corners) {
    if (_refDesc.empty() || frameBgr.empty()) return false;

    cv::Mat gray;
    cv::cvtColor(frameBgr, gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::KeyPoint> kp;
    cv::Mat desc;
    _brisk->detectAndCompute(gray, cv::noArray(), kp, desc);
    if (kp.size() < 8) return false;

    // 2-NN match + Lowe ratio test.
    std::vector<std::vector<cv::DMatch>> knn;
    _matcher->knnMatch(desc, _refDesc, knn, 2);

    std::vector<cv::Point2f> refPts, framePts;
    refPts.reserve(knn.size());
    framePts.reserve(knn.size());
    for (const auto& nn : knn) {
        if (nn.size() < 2) continue;
        if (nn[0].distance < 0.75f * nn[1].distance) {
            refPts.push_back(_refKp[nn[0].trainIdx].pt);
            framePts.push_back(kp[nn[0].queryIdx].pt);
        }
    }
    if (refPts.size() < 8) return false;

    cv::Mat mask;
    cv::Mat H = cv::findHomography(refPts, framePts, cv::RANSAC, 3.0, mask);
    if (!niceHomography(H)) return false;

    // Keep only the inliers for downstream KLT seeding.
    _refMatched.clear();
    _frameMatched.clear();
    for (int i = 0; i < mask.rows; ++i) {
        if (mask.at<uchar>(i)) {
            _refMatched.push_back(refPts[i]);
            _frameMatched.push_back(framePts[i]);
        }
    }
    if (_frameMatched.size() < 8) return false;

    std::vector<cv::Point2f> refQuad(_refCorners.begin(), _refCorners.end());
    std::vector<cv::Point2f> dstQuad;
    cv::perspectiveTransform(refQuad, dstQuad, H);
    if (dstQuad.size() != 4) return false;
    for (int i = 0; i < 4; ++i) corners[i] = dstQuad[i];
    return true;
}

bool BriskTracker::track(const cv::Mat& prevGray, const cv::Mat& currGray,
                         std::array<cv::Point2f, 4>& corners) {
    if (prevGray.empty() || currGray.empty()) return false;
    if (_frameMatched.size() < 8) return false;

    std::vector<cv::Point2f> next;
    std::vector<unsigned char> status;
    std::vector<float> err;
    cv::calcOpticalFlowPyrLK(prevGray, currGray, _frameMatched, next,
                             status, err, cv::Size(21, 21), 3);

    std::vector<cv::Point2f> srcKept, dstKept;
    srcKept.reserve(next.size());
    dstKept.reserve(next.size());
    for (size_t i = 0; i < next.size(); ++i) {
        if (status[i]) {
            srcKept.push_back(_frameMatched[i]);
            dstKept.push_back(next[i]);
        }
    }
    if (srcKept.size() < 8) return false;

    cv::Mat H = cv::findHomography(srcKept, dstKept, cv::RANSAC, 3.0);
    if (!niceHomography(H)) return false;

    std::vector<cv::Point2f> in(corners.begin(), corners.end());
    std::vector<cv::Point2f> out;
    cv::perspectiveTransform(in, out, H);
    if (out.size() != 4) return false;
    for (int i = 0; i < 4; ++i) corners[i] = out[i];
    _frameMatched = dstKept;
    return true;
}

} // namespace baekar
