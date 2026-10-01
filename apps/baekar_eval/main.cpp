// baekar_eval: headless evaluation (roadmap F2). No window or GL.
//
//   baekar_eval markers --marker IMG [--marker IMG ...] [--tracker legacy|multi]
//                       [--frames N] [--realtime] [--fps F] --out DIR
//   baekar_eval trajectory --estimate FILE --reference FILE [--sim3]
//                          [--max-diff S] [--delta N] --out DIR
//   baekar_eval dataset DIR --out DIR
//   baekar_eval model MODEL [--runs N]

#include "adapters/frame_source/DatasetSources.h"
#include "adapters/frame_source/FrameSources.h"
#include "adapters/inference/InferenceEngines.h"
#include "adapters/tracking/MarkerTrackers.h"
#include "evaluation/MarkerBenchmark.h"
#include "evaluation/Metrics.h"
#include "evaluation/Report.h"
#include "evaluation/Trajectory.h"

#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace baekar;

namespace {

int usage() {
    std::fprintf(stderr,
                 "Usage:\n"
                 "  baekar_eval markers --marker IMG [--marker IMG ...] [--tracker legacy|multi]\n"
                 "                      [--frames N] [--realtime] [--fps F] --out DIR\n"
                 "  baekar_eval trajectory --estimate FILE --reference FILE [--sim3]\n"
                 "                         [--max-diff SECONDS] [--delta N] --out DIR\n"
                 "  baekar_eval dataset DIR --out DIR\n"
                 "  baekar_eval model MODEL [--runs N]   (.onnx; .mlmodel/.mlpackage/.mlmodelc on Apple)\n");
    return 2;
}

std::string baseName(const std::string& path) { return fs::path(path).filename().string(); }

int runMarkers(const std::vector<std::string>& args) {
    std::vector<std::string> markers;
    std::string tracker = "auto", out;
    MarkerBenchmarkOptions options;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        auto value = [&]() -> std::string { return i + 1 < args.size() ? args[++i] : std::string(); };
        if (a == "--marker") markers.push_back(value());
        else if (a == "--tracker") tracker = value();
        else if (a == "--frames") options.frames = std::atoi(value().c_str());
        else if (a == "--fps") options.fps = std::atof(value().c_str());
        else if (a == "--realtime") options.realTime = true;
        else if (a == "--out") out = value();
        else return usage();
    }
    if (markers.empty() || out.empty() || options.frames <= 0) return usage();
    const bool multi = tracker == "multi" || (tracker == "auto" && markers.size() > 1);

    std::unique_ptr<IMarkerTracker> markerTracker;
    if (multi) markerTracker = std::make_unique<MultiMarkerTracker>();
    else markerTracker = std::make_unique<LegacySingleMarkerTracker>();
    SyntheticFrameSource source(markers);
    if (!source.open()) {
        std::fprintf(stderr, "baekar_eval: cannot open the synthetic source\n");
        return 1;
    }
    const MarkerBenchmarkResult result = runMarkerBenchmark(source, *markerTracker, markers, options);
    source.close();
    std::vector<std::string> names;
    for (const std::string& m : markers) names.push_back(baseName(m));
    const std::string title = std::to_string(markers.size()) + "-marker benchmark (" +
                              (options.realTime ? "real time" : "every frame") + ")";
    if (!writeMarkerReport(out, title, result, names, options)) return 1;
    std::printf("%s\n  found %.1f %%\n  display error: %s\n  source error:  %s\n  %s: %s\n", title.c_str(),
                100.0 * result.overallFoundRate, formatStats(result.displayError, "px").c_str(),
                formatStats(result.sourceError, "px").c_str(), options.realTime ? "lag" : "latency",
                formatStats(options.realTime ? result.lagFrames : result.latencyMs,
                            options.realTime ? "frames" : "ms").c_str());

    // Camera path from the first marker against the source's reference path.
    if (result.estimate.size() >= 3 && result.reference.size() >= 3) {
        const TrajectoryComparison comparison = compareTrajectories(result.estimate, result.reference, true);
        if (comparison.matchedPoses >= 3) {
            const std::string trajectoryDir = (fs::path(out) / "trajectory").string();
            writeTrajectoryReport(trajectoryDir, "Camera path from " + names.front() + " (" + title + ")",
                                  result.reference, comparison, true);
            writeTumTrajectory((fs::path(trajectoryDir) / "estimate.txt").string(), result.estimate);
            writeTumTrajectory((fs::path(trajectoryDir) / "reference.txt").string(), result.reference);
            std::printf("  camera path (%zu poses, Sim3): ATE %s\n", comparison.matchedPoses,
                        formatStats(comparison.ate, "m").c_str());
        }
    }
    return 0;
}

int runTrajectory(const std::vector<std::string>& args) {
    std::string estimatePath, referencePath, out;
    bool sim3 = false;
    double maxDiff = 0.02;
    std::size_t delta = 1;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        auto value = [&]() -> std::string { return i + 1 < args.size() ? args[++i] : std::string(); };
        if (a == "--estimate") estimatePath = value();
        else if (a == "--reference") referencePath = value();
        else if (a == "--sim3") sim3 = true;
        else if (a == "--max-diff") maxDiff = std::atof(value().c_str());
        else if (a == "--delta") delta = static_cast<std::size_t>(std::atoi(value().c_str()));
        else if (a == "--out") out = value();
        else return usage();
    }
    if (estimatePath.empty() || referencePath.empty() || out.empty() || delta == 0) return usage();
    Trajectory estimate, reference;
    std::string error;
    if (!readTumTrajectory(estimatePath, estimate, &error) || !readTumTrajectory(referencePath, reference, &error)) {
        std::fprintf(stderr, "baekar_eval: %s\n", error.c_str());
        return 1;
    }
    const TrajectoryComparison comparison = compareTrajectories(estimate, reference, sim3, maxDiff, delta);
    if (comparison.matchedPoses < 3) {
        std::fprintf(stderr, "baekar_eval: only %zu poses matched in time\n", comparison.matchedPoses);
        return 1;
    }
    if (!writeTrajectoryReport(out, "Trajectory: " + baseName(estimatePath), reference, comparison, sim3)) return 1;
    std::printf("matched %zu\n  ATE %s\n  RPE %s / %s\n", comparison.matchedPoses,
                formatStats(comparison.ate, "m").c_str(), formatStats(comparison.rpeTranslation, "m").c_str(),
                formatStats(comparison.rpeRotation, "deg").c_str());
    return 0;
}

int runDataset(const std::vector<std::string>& args) {
    std::string directory, out;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--out" && i + 1 < args.size()) out = args[++i];
        else if (directory.empty()) directory = args[i];
        else return usage();
    }
    if (directory.empty() || out.empty()) return usage();
    std::unique_ptr<IFrameSource> source = openDatasetDirectory(directory, /*loop=*/false);
    if (!source || !source->open()) {
        std::fprintf(stderr, "baekar_eval: cannot open dataset %s\n", directory.c_str());
        return 1;
    }
    Trajectory reference;
    std::size_t frames = 0, withDepth = 0, withPose = 0, imuSamples = 0;
    double firstTime = 0, lastTime = 0;
    cv::Mat firstImage;
    std::optional<CameraIntrinsics> intrinsics;
    Frame frame;
    while (source->read(frame)) {
        if (frames == 0) {
            firstTime = frame.timestampSeconds;
            firstImage = frame.bgr.clone();
            intrinsics = frame.intrinsics;
        }
        lastTime = frame.timestampSeconds;
        ++frames;
        if (!frame.depth.empty()) ++withDepth;
        imuSamples += frame.imu.size();
        if (frame.referencePose) {
            ++withPose;
            reference.push_back({frame.timestampSeconds, *frame.referencePose});
        }
    }
    source->close();
    fs::create_directories(out);
    const double duration = lastTime - firstTime;
    std::ofstream md(fs::path(out) / "report.md");
    md << "# Dataset: " << source->describe() << "\n\n";
    md << "- Frames: " << frames << " over " << duration << " s ("
       << (duration > 0 ? (frames - 1) / duration : 0.0) << " fps)\n";
    md << "- With depth: " << withDepth << "\n- With reference pose: " << withPose << "\n";
    md << "- IMU samples: " << imuSamples << "\n";
    if (intrinsics)
        md << "- Intrinsics: fx " << intrinsics->fx << ", fy " << intrinsics->fy << ", cx " << intrinsics->cx
           << ", cy " << intrinsics->cy << ", " << intrinsics->width << "x" << intrinsics->height << "\n";
    if (!firstImage.empty()) {
        cv::imwrite((fs::path(out) / "first_frame.png").string(), firstImage);
        md << "\n![first frame](first_frame.png)\n";
    }
    if (reference.size() >= 2) {
        writeTumTrajectory((fs::path(out) / "reference.txt").string(), reference);
        cv::imwrite((fs::path(out) / "reference.png").string(),
                    plotTrajectories({{reference, cv::Scalar(60, 60, 60)}}, "reference trajectory"));
        md << "\n![reference trajectory](reference.png)\n";
    }
    std::printf("%s: %zu frames, %zu with depth, %zu with pose, %zu IMU samples\n", source->describe().c_str(),
                frames, withDepth, withPose, imuSamples);
    return frames > 0 ? 0 : 1;
}

std::string shapeText(const std::vector<std::int64_t>& shape) {
    std::string text;
    for (std::size_t i = 0; i < shape.size(); ++i) text += (i ? "x" : "") + (shape[i] < 0 ? std::string("?") : std::to_string(shape[i]));
    return text.empty() ? "scalar" : text;
}

// Prints a model's inputs and outputs and times inference on zero inputs
// (open dimensions set to 1).
int runModel(const std::vector<std::string>& args) {
    std::string path;
    int runs = 20;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--runs" && i + 1 < args.size()) runs = std::atoi(args[++i].c_str());
        else if (path.empty()) path = args[i];
        else return usage();
    }
    if (path.empty() || runs <= 0) return usage();
    std::string error;
    std::unique_ptr<IInferenceEngine> engine = openInferenceModel(path, &error);
    if (!engine) {
        std::fprintf(stderr, "baekar_eval: %s\n", error.c_str());
        return 1;
    }
    std::printf("%s\n", engine->describe().c_str());
    std::vector<Tensor> inputs;
    for (const TensorInfo& info : engine->inputs()) {
        std::printf("  input  %-20s %s\n", info.name.c_str(), shapeText(info.shape).c_str());
        Tensor t;
        t.name = info.name;
        for (std::int64_t d : info.shape) t.shape.push_back(d < 0 ? 1 : d);
        t.data.assign(Tensor::elementCount(t.shape), 0.0f);
        inputs.push_back(std::move(t));
    }
    for (const TensorInfo& info : engine->outputs())
        std::printf("  output %-20s %s\n", info.name.c_str(), shapeText(info.shape).c_str());

    std::vector<Tensor> outputs;
    std::vector<double> times;
    for (int i = 0; i <= runs; ++i) {  // the first run (warm-up) is not timed
        const auto start = std::chrono::steady_clock::now();
        if (!engine->run(inputs, outputs, &error)) {
            std::fprintf(stderr, "baekar_eval: %s\n", error.c_str());
            return 1;
        }
        if (i > 0) times.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
    }
    std::printf("  %d runs: %s\n", runs, formatStats(summarize(times), "ms").c_str());
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) return usage();
    const std::string command = argv[1];
    const std::vector<std::string> args(argv + 2, argv + argc);
    if (command == "markers") return runMarkers(args);
    if (command == "trajectory") return runTrajectory(args);
    if (command == "dataset") return runDataset(args);
    if (command == "model") return runModel(args);
    return usage();
}
