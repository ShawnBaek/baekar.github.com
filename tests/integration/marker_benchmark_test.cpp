// Marker benchmark on the synthetic camera, which knows where every marker
// is. Every-frame mode waits for each result, so it measures accuracy and is
// stable on a loaded CI machine; real-time pacing depends on CPU speed and
// is only checked for consistency.

#include "adapters/frame_source/FrameSources.h"
#include "adapters/tracking/MarkerTrackers.h"
#include "evaluation/MarkerBenchmark.h"
#include "evaluation/Metrics.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace baekar;

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

// Reports the true corners of each frame: isolates the benchmark and PnP
// from tracking error.
class PerfectTracker final : public IMarkerTracker {
public:
    bool start(const std::vector<std::string>&, const Frame& firstFrame) override {
        submit(firstFrame);
        return true;
    }
    void submit(const Frame& frame) override {
        latest_.clear();
        for (std::size_t m = 0; m < frame.referenceMarkerCorners.size(); ++m) {
            MarkerObservation o;
            o.markerIndex = m;
            o.found = true;
            o.detectionCorners = o.poseCorners = frame.referenceMarkerCorners[m];
            o.frameSequence = frame.sequence;
            latest_.push_back(o);
        }
    }
    std::vector<MarkerObservation> latest() const override { return latest_; }
    void stop() override {}
    bool drivesPose() const override { return true; }
    std::string describe() const override { return "perfect tracker"; }

private:
    std::vector<MarkerObservation> latest_;
};

MarkerBenchmarkResult run(IMarkerTracker& tracker, const std::vector<std::string>& markers,
                          const MarkerBenchmarkOptions& options) {
    SyntheticFrameSource source(markers);
    if (!source.open()) return {};
    MarkerBenchmarkResult result = runMarkerBenchmark(source, tracker, markers, options);
    source.close();
    std::fprintf(stderr, "%s, %zu marker(s), %s: found %.1f %%, source error median %.2f px (n=%zu)\n",
                 result.tracker.c_str(), markers.size(), options.realTime ? "real time" : "every frame",
                 100.0 * result.overallFoundRate, result.sourceError.median, result.sourceError.count);
    return result;
}

}  // namespace

int main() {
    const std::string dir = std::string(BAEKAR_SOURCE_DIR) + "/assets/image";

    // Exact corners: zero image error and, after Sim(3) alignment, the
    // reference camera path.
    {
        PerfectTracker tracker;
        MarkerBenchmarkOptions options;
        options.frames = 60;
        const MarkerBenchmarkResult r = run(tracker, {dir + "/yejin.jpg"}, options);
        expect(r.overallFoundRate == 1.0 && r.sourceError.max < 1e-6, "perfect: zero corner error");
        expect(r.estimate.size() == 60 && r.reference.size() == 60, "perfect: one pose per frame");
        const TrajectoryComparison c = compareTrajectories(r.estimate, r.reference, true);
        std::fprintf(stderr, "perfect tracker: ATE RMSE %.6f m, scale %.4f\n", c.ate.rmse, c.alignment.scale);
        expect(c.matchedPoses == 60 && c.ate.rmse < 1e-3, "perfect: camera path recovered (ATE < 1 mm)");
        expect(c.rpeRotation.max < 0.05, "perfect: rotation recovered");
    }

    // Single marker, 2012 pipeline. Measured: 100 % found, 0.9 px.
    {
        LegacySingleMarkerTracker tracker;
        MarkerBenchmarkOptions options;
        options.frames = 120;
        const MarkerBenchmarkResult r = run(tracker, {dir + "/yejin.jpg"}, options);
        expect(r.scoredFrames == 120, "legacy: all frames scored");
        expect(r.overallFoundRate >= 0.9, "legacy: marker found in >= 90 % of frames");
        expect(r.sourceError.count > 0 && r.sourceError.median < 3.0, "legacy: median corner error < 3 px");
        expect(r.lagFrames.count > 0 && r.lagFrames.max <= 1.0, "legacy: every-frame results are for that frame");
        // Measured ATE RMSE 0.13 m at 4-6 m from a 1.7 m marker.
        const TrajectoryComparison c = compareTrajectories(r.estimate, r.reference, true);
        expect(c.matchedPoses >= 100 && c.ate.rmse < 0.3, "legacy: camera path ATE RMSE < 0.3 m");
    }

    // Three markers, synchronized tracker.
    {
        MultiMarkerTracker tracker;
        MarkerBenchmarkOptions options;
        options.frames = 120;
        const std::vector<std::string> markers = {dir + "/yejin.jpg", dir + "/iu1.jpg", dir + "/cola.jpg"};
        const MarkerBenchmarkResult r = run(tracker, markers, options);
        expect(r.foundRate.size() == 3, "multi: one rate per marker");
        expect(r.overallFoundRate >= 0.8, "multi: markers found in >= 80 % of frames");
        expect(r.sourceError.count > 0 && r.sourceError.median < 8.0, "multi: median corner error < 8 px");
        expect(r.lagFrames.count > 0 && r.lagFrames.median == 0.0, "multi: sequence numbers map to frames");
    }

    // Real-time pacing: results may lag, but they must refer to real frames
    // and be as accurate for the frame they were computed on.
    {
        LegacySingleMarkerTracker tracker;
        MarkerBenchmarkOptions options;
        options.frames = 90;
        options.realTime = true;
        const MarkerBenchmarkResult r = run(tracker, {dir + "/yejin.jpg"}, options);
        expect(r.scoredFrames == 90, "real time: all frames scored");
        expect(r.lagFrames.count == 0 || r.lagFrames.median >= 0.0, "real time: lag is not negative");
        expect(r.sourceError.count == 0 || r.sourceError.median < 3.0, "real time: source error stays small");
    }

    if (failures == 0) std::fprintf(stderr, "marker_benchmark_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
