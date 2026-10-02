// Trajectory metrics against cases with a known answer.

#include "evaluation/Metrics.h"
#include "evaluation/Trajectory.h"

#include <opencv2/calib3d.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>

using namespace baekar;

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

cv::Matx33d rotation(double rx, double ry, double rz) {
    cv::Matx33d R;
    cv::Rodrigues(cv::Vec3d(rx, ry, rz), R);
    return R;
}

// A camera moving on a curve and turning, 10 Hz for 20 s.
Trajectory makeReference() {
    Trajectory t;
    for (int i = 0; i < 200; ++i) {
        const double s = i * 0.1;
        TimedPose p;
        p.timestampSeconds = 100.0 + s;
        p.pose.t = cv::Vec3d(std::cos(s * 0.3) * 2.0, 0.5 * std::sin(s * 0.7), s * 0.1);
        p.pose.R = rotation(0.1 * std::sin(s), 0.3 * s, 0.05 * std::cos(s));
        t.push_back(p);
    }
    return t;
}

// estimate = inverse of (world alignment with scale): what a scale-free
// monocular system in its own frame would report.
Trajectory transformTrajectory(const Trajectory& in, const cv::Matx33d& R, const cv::Vec3d& t, double scale) {
    Trajectory out = in;
    for (TimedPose& p : out) {
        p.pose.t = scale * (R * p.pose.t) + t;
        p.pose.R = R * p.pose.R;
    }
    return out;
}

}  // namespace

int main() {
    // Quaternion round trip.
    {
        const cv::Matx33d R = rotation(0.3, -1.2, 2.0);
        const cv::Vec4d q = rotationToQuaternion(R);
        const cv::Matx33d back = quaternionToRotation(q[0], q[1], q[2], q[3]);
        expect(cv::norm(cv::Mat(R), cv::Mat(back)) < 1e-9, "quaternion round trip");
        expect(std::abs(cv::norm(q) - 1.0) < 1e-12, "unit quaternion");
    }

    const Trajectory reference = makeReference();

    // Umeyama recovers a known similarity exactly.
    {
        const cv::Matx33d R = rotation(0.4, 0.2, -0.7);
        const cv::Vec3d t(1.0, -2.0, 0.5);
        std::vector<cv::Vec3d> est, ref;
        for (const TimedPose& p : reference) {
            ref.push_back(p.pose.t);
            est.push_back((R.t() * (p.pose.t - t)) * (1.0 / 2.5));
        }
        const Alignment a = umeyamaAlignment(est, ref, true);
        expect(std::abs(a.scale - 2.5) < 1e-9, "Umeyama scale");
        expect(cv::norm(cv::Mat(a.R), cv::Mat(R)) < 1e-9, "Umeyama rotation");
        expect(cv::norm(a.t - t) < 1e-9, "Umeyama translation");
    }

    // A scaled, moved copy has zero error after Sim(3) alignment.
    {
        const Trajectory estimate = transformTrajectory(reference, rotation(0.2, 0.9, -0.3), cv::Vec3d(5, 1, -2), 0.37);
        const TrajectoryComparison c = compareTrajectories(estimate, reference, true);
        expect(c.matchedPoses == reference.size(), "all poses matched");
        expect(c.ate.rmse < 1e-9, "exact copy: ATE 0");
        expect(c.rpeTranslation.rmse < 1e-9 && c.rpeRotation.rmse < 1e-6, "exact copy: RPE 0");
        expect(std::abs(c.alignment.scale - 1.0 / 0.37) < 1e-9, "recovered monocular scale");
        const TrajectoryComparison rigid = compareTrajectories(estimate, reference, false);
        expect(rigid.ate.rmse > 0.1, "SE(3) alignment cannot hide a wrong scale");
    }

    // A constant 5 cm offset along one axis in the camera path gives 5 cm ATE
    // for SE(3) only if the alignment is not allowed to absorb it; the mean
    // of a zero-mean alternating error is what remains.
    {
        Trajectory estimate = reference;
        for (std::size_t i = 0; i < estimate.size(); ++i) estimate[i].pose.t[1] += (i % 2 ? 0.05 : -0.05);
        const TrajectoryComparison c = compareTrajectories(estimate, reference, false);
        expect(std::abs(c.ate.rmse - 0.05) < 2e-3, "alternating 5 cm error gives ATE RMSE of 5 cm");
        expect(std::abs(c.rpeTranslation.median - 0.10) < 2e-3, "alternating 5 cm error gives RPE of 10 cm");
    }

    // Time association tolerates offsets and does not reuse poses.
    {
        Trajectory shifted = reference;
        for (TimedPose& p : shifted) p.timestampSeconds += 0.004;
        expect(associateByTime(shifted, reference, 0.02).size() == reference.size(), "4 ms offset matches all");
        expect(associateByTime(shifted, reference, 0.001).empty(), "4 ms offset beyond tolerance matches none");
    }

    // TUM text round trip.
    {
        const std::string path = (std::filesystem::temp_directory_path() / "baekar_traj_test.txt").string();
        expect(writeTumTrajectory(path, reference), "write TUM trajectory");
        Trajectory back;
        expect(readTumTrajectory(path, back), "read TUM trajectory");
        expect(back.size() == reference.size(), "same length");
        const TrajectoryComparison c = compareTrajectories(back, reference, false);
        expect(c.ate.rmse < 1e-6 && c.rpeRotation.max < 1e-4, "round trip keeps poses");
        std::filesystem::remove(path);
    }

    if (failures == 0) std::fprintf(stderr, "trajectory_metrics_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
