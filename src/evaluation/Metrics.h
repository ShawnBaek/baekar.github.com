#ifndef BAEKAR_EVALUATION_METRICS_H
#define BAEKAR_EVALUATION_METRICS_H

#include "evaluation/Trajectory.h"

#include <vector>

namespace baekar {

struct ErrorStats {
    std::size_t count = 0;
    double rmse = 0, mean = 0, median = 0, p95 = 0, max = 0;
};
ErrorStats summarize(std::vector<double> values);

// Similarity (or rigid, scale = 1) transform that maps estimate points onto
// reference points in the least-squares sense (Umeyama 1991).
struct Alignment {
    cv::Matx33d R = cv::Matx33d::eye();
    cv::Vec3d t{0, 0, 0};
    double scale = 1.0;
    cv::Vec3d apply(const cv::Vec3d& p) const { return scale * (R * p) + t; }
};
Alignment umeyamaAlignment(const std::vector<cv::Vec3d>& estimate,
                           const std::vector<cv::Vec3d>& reference, bool withScale);

struct TrajectoryComparison {
    std::size_t matchedPoses = 0;
    Alignment alignment;
    ErrorStats ate;             // absolute trajectory error, metres
    ErrorStats rpeTranslation;  // relative pose error, metres per step
    ErrorStats rpeRotation;     // degrees per step
    std::vector<double> ateTimestamps, atePerPose;
    Trajectory alignedEstimate;
};

// ATE after aligning the estimate to the reference (Sim(3) for monocular
// scale-free estimates, SE(3) otherwise), and RPE over `rpeDelta` matched
// poses. Pairs poses within maxTimeDifference seconds.
TrajectoryComparison compareTrajectories(const Trajectory& estimate, const Trajectory& reference,
                                         bool withScale, double maxTimeDifference = 0.02,
                                         std::size_t rpeDelta = 1);

// Rotation angle of R in degrees.
double rotationAngleDegrees(const cv::Matx33d& R);

}  // namespace baekar

#endif  // BAEKAR_EVALUATION_METRICS_H
