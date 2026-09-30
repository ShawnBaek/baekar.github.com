#include "evaluation/Metrics.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace baekar {

ErrorStats summarize(std::vector<double> values) {
    ErrorStats s;
    s.count = values.size();
    if (values.empty()) return s;
    std::sort(values.begin(), values.end());
    double sum = 0, sumSquares = 0;
    for (double v : values) {
        sum += v;
        sumSquares += v * v;
    }
    s.mean = sum / values.size();
    s.rmse = std::sqrt(sumSquares / values.size());
    const std::size_t n = values.size();
    s.median = n % 2 ? values[n / 2] : 0.5 * (values[n / 2 - 1] + values[n / 2]);
    s.p95 = values[std::min(n - 1, static_cast<std::size_t>(std::ceil(0.95 * n)) - 1)];
    s.max = values.back();
    return s;
}

Alignment umeyamaAlignment(const std::vector<cv::Vec3d>& estimate,
                           const std::vector<cv::Vec3d>& reference, bool withScale) {
    Alignment a;
    const std::size_t n = std::min(estimate.size(), reference.size());
    if (n < 3) return a;

    cv::Vec3d meanE(0, 0, 0), meanR(0, 0, 0);
    for (std::size_t i = 0; i < n; ++i) {
        meanE += estimate[i];
        meanR += reference[i];
    }
    meanE *= 1.0 / n;
    meanR *= 1.0 / n;

    cv::Matx33d sigma = cv::Matx33d::zeros();
    double varianceE = 0;
    for (std::size_t i = 0; i < n; ++i) {
        const cv::Vec3d e = estimate[i] - meanE;
        const cv::Vec3d r = reference[i] - meanR;
        sigma += cv::Matx33d(r[0] * e[0], r[0] * e[1], r[0] * e[2],
                             r[1] * e[0], r[1] * e[1], r[1] * e[2],
                             r[2] * e[0], r[2] * e[1], r[2] * e[2]);
        varianceE += e.dot(e);
    }
    sigma *= 1.0 / n;
    varianceE /= n;

    cv::Mat w, u, vt;
    cv::SVD::compute(cv::Mat(sigma), w, u, vt);
    cv::Matx33d U(u), Vt(vt);
    cv::Matx33d S = cv::Matx33d::eye();
    if (cv::determinant(U) * cv::determinant(Vt) < 0) S(2, 2) = -1;
    a.R = U * S * Vt;
    if (withScale && varianceE > 0) {
        const double traceDS = w.at<double>(0) * S(0, 0) + w.at<double>(1) * S(1, 1) + w.at<double>(2) * S(2, 2);
        a.scale = traceDS / varianceE;
    }
    a.t = meanR - a.scale * (a.R * meanE);
    return a;
}

double rotationAngleDegrees(const cv::Matx33d& R) {
    // atan2 of sin and cos stays accurate near 0 and 180 degrees, where
    // acos((trace - 1) / 2) loses precision.
    const double sine = 0.5 * std::sqrt((R(2, 1) - R(1, 2)) * (R(2, 1) - R(1, 2)) +
                                        (R(0, 2) - R(2, 0)) * (R(0, 2) - R(2, 0)) +
                                        (R(1, 0) - R(0, 1)) * (R(1, 0) - R(0, 1)));
    const double cosine = (R(0, 0) + R(1, 1) + R(2, 2) - 1.0) / 2.0;
    return std::atan2(sine, cosine) * 180.0 / CV_PI;
}

TrajectoryComparison compareTrajectories(const Trajectory& estimate, const Trajectory& reference,
                                         bool withScale, double maxTimeDifference,
                                         std::size_t rpeDelta) {
    TrajectoryComparison result;
    const auto pairs = associateByTime(estimate, reference, maxTimeDifference);
    result.matchedPoses = pairs.size();
    if (pairs.size() < 3) return result;

    std::vector<cv::Vec3d> e, r;
    for (const auto& p : pairs) {
        e.push_back(estimate[p.first].pose.t);
        r.push_back(reference[p.second].pose.t);
    }
    result.alignment = umeyamaAlignment(e, r, withScale);

    std::vector<double> ate;
    for (std::size_t k = 0; k < pairs.size(); ++k) {
        const cv::Vec3d aligned = result.alignment.apply(e[k]);
        ate.push_back(cv::norm(aligned - r[k]));
        result.ateTimestamps.push_back(reference[pairs[k].second].timestampSeconds);
        TimedPose alignedPose;
        alignedPose.timestampSeconds = estimate[pairs[k].first].timestampSeconds;
        alignedPose.pose.R = result.alignment.R * estimate[pairs[k].first].pose.R;
        alignedPose.pose.t = aligned;
        result.alignedEstimate.push_back(alignedPose);
    }
    result.atePerPose = ate;
    result.ate = summarize(ate);

    std::vector<double> rpeT, rpeR;
    for (std::size_t k = 0; k + rpeDelta < pairs.size(); ++k) {
        const RigidTransform& ea = estimate[pairs[k].first].pose;
        const RigidTransform& eb = estimate[pairs[k + rpeDelta].first].pose;
        const RigidTransform& ra = reference[pairs[k].second].pose;
        const RigidTransform& rb = reference[pairs[k + rpeDelta].second].pose;
        RigidTransform estimateStep = ea.inverse() * eb;
        estimateStep.t *= result.alignment.scale;  // monocular scale
        const RigidTransform referenceStep = ra.inverse() * rb;
        const RigidTransform error = referenceStep.inverse() * estimateStep;
        rpeT.push_back(cv::norm(error.t));
        rpeR.push_back(rotationAngleDegrees(error.R));
    }
    result.rpeTranslation = summarize(rpeT);
    result.rpeRotation = summarize(rpeR);
    return result;
}

}  // namespace baekar
