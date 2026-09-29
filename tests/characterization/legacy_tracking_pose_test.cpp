// Characterization of the 2012 single-marker pipeline after it moved behind
// IMarkerTracker and IPoseEstimator: it must still find the synthetic
// marker, and the CCamera pose must stay deterministic.

#include "adapters/frame_source/FrameSources.h"
#include "adapters/pose/LegacyCameraPoseEstimator.h"
#include "adapters/tracking/MarkerTrackers.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

// Convex, non-degenerate quadrilateral near the frame.
bool plausibleQuad(const std::array<cv::Point2f, 4>& q) {
    double sign = 0;
    for (int i = 0; i < 4; ++i) {
        const cv::Point2f a = q[i], b = q[(i + 1) % 4], c = q[(i + 2) % 4];
        const double cross = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
        if (std::abs(cross) < 1.0) return false;
        if (sign != 0 && (cross > 0) != (sign > 0)) return false;
        sign = cross;
        if (a.x < -320 || a.x > 960 || a.y < -240 || a.y > 720) return false;
    }
    return true;
}

}  // namespace

int main() {
    const std::string root = BAEKAR_SOURCE_DIR;
    const std::string marker = root + "/MarkerlessAR/image/yejin.jpg";

    // ---- tracker -------------------------------------------------------
    baekar::SyntheticFrameSource source({marker});
    expect(source.open(), "synthetic source opens");
    baekar::Frame frame;
    expect(source.read(frame), "first frame");

    baekar::LegacySingleMarkerTracker tracker;
    expect(tracker.drivesPose(), "legacy tracker drives the pose");
    expect(tracker.start({marker}, frame), "legacy tracker starts");

    int observed = 0, found = 0, plausible = 0;
    baekar::MarkerObservation lastFound;
    for (int i = 0; i < 150; ++i) {
        expect(source.read(frame), "synthetic frame");
        tracker.submit(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        if (i < 20) continue;  // let the workers see a few frames first
        const std::vector<baekar::MarkerObservation> latest = tracker.latest();
        expect(latest.size() == 1, "one observation");
        ++observed;
        if (latest[0].found) {
            ++found;
            lastFound = latest[0];
            if (plausibleQuad(latest[0].poseCorners)) ++plausible;
        }
    }
    tracker.stop();
    std::fprintf(stderr, "legacy tracker: found %d/%d, plausible corners %d/%d\n", found, observed,
                 plausible, found);
    expect(found >= observed * 9 / 10, "marker found in at least 90% of frames");
    expect(plausible >= found * 9 / 10, "pose corners form a plausible quadrilateral");

    // ---- pose ----------------------------------------------------------
    baekar::LegacyCameraPoseEstimator pose;
    expect(pose.loadCalibration(root + "/MarkerlessAR/calibration/calibration.txt"), "calibration loads");

    // The 2012 projection: (2n/(r-l)) with r-l = 640/fx, fx = 664.006287.
    const baekar::Mat4 projection = pose.projection();
    expect(std::abs(projection.m[0] - 2.0 * 664.006287 / 640.0) < 1e-3, "projection[0] from fx");
    expect(std::abs(projection.m[5] - 2.0 * 673.420898 / 480.0) < 1e-3, "projection[5] from fy");

    expect(lastFound.found, "have a found observation for the pose");
    const baekar::Pose first = pose.estimate(lastFound);
    const baekar::Pose second = pose.estimate(lastFound);
    expect(first.valid && second.valid, "pose valid");
    bool finite = true, same = true;
    for (int i = 0; i < 16; ++i) {
        finite = finite && std::isfinite(first.view.m[i]);
        same = same && first.view.m[i] == second.view.m[i];
    }
    expect(finite, "view matrix is finite");
    expect(same, "same corners give the same view matrix");

    baekar::MarkerObservation lost;
    expect(!pose.estimate(lost).valid, "no pose without a marker");

    if (failures == 0) std::fprintf(stderr, "legacy_tracking_pose_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
