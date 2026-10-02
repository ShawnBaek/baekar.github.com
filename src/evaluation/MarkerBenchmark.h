#ifndef BAEKAR_EVALUATION_MARKER_BENCHMARK_H
#define BAEKAR_EVALUATION_MARKER_BENCHMARK_H

#include "application/ports/IFrameSource.h"
#include "application/ports/IMarkerTracker.h"
#include "evaluation/Metrics.h"
#include "evaluation/Trajectory.h"

#include <string>
#include <vector>

namespace baekar {

struct MarkerBenchmarkOptions {
    int frames = 300;
    // every-frame: wait for the tracker to publish each frame (algorithm
    // accuracy and latency). realtime: submit at `fps` like the app does and
    // let the tracker skip frames (what the user sees).
    bool realTime = false;
    double fps = 30.0;
    int warmupFrames = 30;  // not scored: initial acquisition
};

struct MarkerSample {
    std::uint64_t frame = 0;
    std::size_t marker = 0;
    bool found = false;
    double displayError = -1;  // px, against the frame shown with this snapshot
    double sourceError = -1;   // px, against the frame the tracker processed
    long lagFrames = 0;        // shown frame - processed frame
};

struct MarkerBenchmarkResult {
    std::string tracker;
    std::size_t markers = 0;
    int scoredFrames = 0;
    std::vector<double> foundRate;      // per marker
    std::vector<double> meanDisplayError;  // per marker, px
    double overallFoundRate = 0;
    ErrorStats displayError;  // px, found samples
    ErrorStats sourceError;   // px, found samples with a known processed frame
    ErrorStats latencyMs;     // every-frame mode: submit -> published
    ErrorStats lagFrames;     // realtime mode
    std::vector<MarkerSample> samples;

    // Camera path from the first marker's corners (PnP on a marker of unit
    // height, so the scale is arbitrary: compare with Sim(3) alignment),
    // timestamped with the frame each result was computed on, and the
    // source's reference path. Empty without intrinsics or reference poses.
    Trajectory estimate;
    Trajectory reference;
};

// Runs `tracker` on a source that fills Frame::referenceMarkerCorners (the
// synthetic source) and scores the drawn outline (detectionCorners). With
// Frame::intrinsics and Frame::referencePose it also records trajectories.
MarkerBenchmarkResult runMarkerBenchmark(IFrameSource& source, IMarkerTracker& tracker,
                                         const std::vector<std::string>& markerImages,
                                         const MarkerBenchmarkOptions& options);

// Mean distance between corresponding corners, in pixels.
double cornerError(const std::array<cv::Point2f, 4>& a, const std::array<cv::Point2f, 4>& b);

}  // namespace baekar

#endif  // BAEKAR_EVALUATION_MARKER_BENCHMARK_H
