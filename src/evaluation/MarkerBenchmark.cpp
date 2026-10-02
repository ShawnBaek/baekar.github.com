#include "evaluation/MarkerBenchmark.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>

#include <chrono>
#include <map>
#include <thread>

namespace baekar {
namespace {

struct SourceFrame {
    std::vector<std::array<cv::Point2f, 4>> corners;
    double timestampSeconds = 0;
};

// Marker outline in marker units: height 1, width = image aspect, centred,
// clockwise from the top-left like the image corners.
std::vector<cv::Point3d> unitMarkerOutline(const std::string& image) {
    const cv::Mat marker = cv::imread(image, cv::IMREAD_UNCHANGED);
    const double half = 0.5, halfWidth = marker.empty() ? 0.5 : 0.5 * marker.cols / marker.rows;
    return {{-halfWidth, -half, 0}, {halfWidth, -half, 0}, {halfWidth, half, 0}, {-halfWidth, half, 0}};
}

bool cameraPose(const std::vector<cv::Point3d>& outline, const std::array<cv::Point2f, 4>& corners,
                const CameraIntrinsics& k, RigidTransform& worldFromCamera) {
    std::vector<cv::Point2d> image(corners.begin(), corners.end());
    cv::Vec3d rvec, tvec;
    if (!cv::solvePnP(outline, image, cv::Mat(k.matrix()), cv::Mat(std::vector<double>(k.distortion.begin(), k.distortion.end())), rvec, tvec, false,
                      cv::SOLVEPNP_IPPE))
        return false;
    RigidTransform cameraFromMarker;
    cv::Rodrigues(rvec, cameraFromMarker.R);
    cameraFromMarker.t = tvec;
    worldFromCamera = cameraFromMarker.inverse();
    return true;
}

}  // namespace

double cornerError(const std::array<cv::Point2f, 4>& a, const std::array<cv::Point2f, 4>& b) {
    double sum = 0;
    for (int i = 0; i < 4; ++i) sum += cv::norm(a[i] - b[i]);
    return sum / 4.0;
}

MarkerBenchmarkResult runMarkerBenchmark(IFrameSource& source, IMarkerTracker& tracker,
                                         const std::vector<std::string>& markerImages,
                                         const MarkerBenchmarkOptions& options) {
    using Clock = std::chrono::steady_clock;
    MarkerBenchmarkResult result;
    result.tracker = tracker.describe();
    result.markers = markerImages.size();

    Frame frame;
    if (!source.read(frame) || !tracker.start(markerImages, frame)) return result;

    std::map<std::uint64_t, SourceFrame> truthBySequence;
    const std::vector<cv::Point3d> outline = markerImages.empty() ? std::vector<cv::Point3d>()
                                                                   : unitMarkerOutline(markerImages.front());
    std::uint64_t lastPoseSequence = 0;
    std::vector<double> display, sourceErrors, latency, lag;
    std::vector<int> foundCount(result.markers, 0);
    std::vector<double> displaySum(result.markers, 0.0);
    const auto framePeriod = std::chrono::duration<double>(1.0 / options.fps);
    auto nextSubmit = Clock::now();

    for (int index = 0; index < options.warmupFrames + options.frames; ++index) {
        if (!source.read(frame)) break;
        truthBySequence[frame.sequence] = {frame.referenceMarkerCorners, frame.timestampSeconds};
        if (truthBySequence.size() > 256) truthBySequence.erase(truthBySequence.begin());

        if (options.realTime) std::this_thread::sleep_until(nextSubmit);
        const auto submitted = Clock::now();
        tracker.submit(frame);
        nextSubmit = submitted + std::chrono::duration_cast<Clock::duration>(framePeriod);

        std::vector<MarkerObservation> observations = tracker.latest();
        if (!options.realTime) {
            // Wait for this frame's snapshot (bounded, so a tracker that only
            // publishes on success cannot stall the run).
            const auto deadline = submitted + std::chrono::milliseconds(500);
            while ((observations.empty() || observations.front().frameSequence < frame.sequence) &&
                   Clock::now() < deadline) {
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                observations = tracker.latest();
            }
            if (!observations.empty() && observations.front().frameSequence >= frame.sequence)
                latency.push_back(std::chrono::duration<double, std::milli>(Clock::now() - submitted).count());
        }
        if (index < options.warmupFrames) continue;
        ++result.scoredFrames;
        if (frame.referencePose) result.reference.push_back({frame.timestampSeconds, *frame.referencePose});

        for (const MarkerObservation& observation : observations) {
            const std::size_t m = observation.markerIndex;
            if (m >= result.markers || m >= frame.referenceMarkerCorners.size()) continue;
            MarkerSample sample;
            sample.frame = frame.sequence;
            sample.marker = m;
            sample.found = observation.found;
            if (observation.found) {
                ++foundCount[m];
                sample.displayError = cornerError(observation.detectionCorners, frame.referenceMarkerCorners[m]);
                display.push_back(sample.displayError);
                displaySum[m] += sample.displayError;
                const auto truth = truthBySequence.find(observation.frameSequence);
                if (truth != truthBySequence.end() && m < truth->second.corners.size()) {
                    sample.sourceError = cornerError(observation.detectionCorners, truth->second.corners[m]);
                    sourceErrors.push_back(sample.sourceError);
                    sample.lagFrames = static_cast<long>(frame.sequence - observation.frameSequence);
                    lag.push_back(static_cast<double>(sample.lagFrames));
                    RigidTransform pose;
                    if (m == 0 && frame.intrinsics && observation.frameSequence > lastPoseSequence &&
                        cameraPose(outline, observation.detectionCorners, *frame.intrinsics, pose)) {
                        result.estimate.push_back({truth->second.timestampSeconds, pose});
                        lastPoseSequence = observation.frameSequence;
                    }
                }
            }
            result.samples.push_back(sample);
        }
    }
    tracker.stop();

    double foundTotal = 0;
    for (std::size_t m = 0; m < result.markers; ++m) {
        const double rate = result.scoredFrames ? static_cast<double>(foundCount[m]) / result.scoredFrames : 0.0;
        result.foundRate.push_back(rate);
        result.meanDisplayError.push_back(foundCount[m] ? displaySum[m] / foundCount[m] : -1.0);
        foundTotal += rate;
    }
    result.overallFoundRate = result.markers ? foundTotal / result.markers : 0.0;
    result.displayError = summarize(display);
    result.sourceError = summarize(sourceErrors);
    result.latencyMs = summarize(latency);
    result.lagFrames = summarize(lag);
    return result;
}

}  // namespace baekar
