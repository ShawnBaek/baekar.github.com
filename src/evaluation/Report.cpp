#include "evaluation/Report.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace baekar {
namespace {

std::string number(double value, int digits = 3) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(digits) << value;
    return s.str();
}

bool ensureDirectory(const std::string& directory) {
    std::error_code error;
    fs::create_directories(directory, error);
    return !error;
}

}  // namespace

std::string formatStats(const ErrorStats& s, const char* unit) {
    if (s.count == 0) return "n/a";
    return "RMSE " + number(s.rmse) + " " + unit + ", mean " + number(s.mean) + ", median " +
           number(s.median) + ", p95 " + number(s.p95) + ", max " + number(s.max) + " (n=" +
           std::to_string(s.count) + ")";
}

bool writeMarkerReport(const std::string& directory, const std::string& title,
                       const MarkerBenchmarkResult& result, const std::vector<std::string>& markerNames,
                       const MarkerBenchmarkOptions& options) {
    if (!ensureDirectory(directory)) return false;
    std::ofstream md(fs::path(directory) / "report.md");
    md << "# " << title << "\n\n";
    md << "- Tracker: " << result.tracker << "\n";
    md << "- Pacing: " << (options.realTime ? "real time at " + number(options.fps, 1) + " fps (frames may be skipped, as in the app)"
                                            : "every frame (the tracker finishes each frame before the next)") << "\n";
    md << "- Scored frames: " << result.scoredFrames << " after " << options.warmupFrames << " warm-up frames\n";
    md << "- Found rate (mean over markers): " << number(100.0 * result.overallFoundRate, 1) << " %\n";
    md << "- Outline error vs. the frame on screen: " << formatStats(result.displayError, "px") << "\n";
    md << "- Outline error vs. the frame the tracker processed: " << formatStats(result.sourceError, "px") << "\n";
    if (!options.realTime) md << "- Latency, submit to published: " << formatStats(result.latencyMs, "ms") << "\n";
    else md << "- Lag, frames between processed and shown: " << formatStats(result.lagFrames, "frames") << "\n";
    md << "\n| # | Marker | Found | Mean outline error (px) |\n|---|---|---|---|\n";
    for (std::size_t m = 0; m < result.markers; ++m) {
        md << "| " << m + 1 << " | " << (m < markerNames.size() ? markerNames[m] : "") << " | "
           << number(100.0 * result.foundRate[m], 1) << " % | "
           << (result.meanDisplayError[m] < 0 ? std::string("n/a") : number(result.meanDisplayError[m], 2)) << " |\n";
    }
    std::ofstream csv(fs::path(directory) / "samples.csv");
    csv << "frame,marker,found,display_error_px,source_error_px,lag_frames\n";
    for (const MarkerSample& s : result.samples)
        csv << s.frame << ',' << s.marker << ',' << s.found << ',' << s.displayError << ',' << s.sourceError
            << ',' << s.lagFrames << '\n';
    return static_cast<bool>(md) && static_cast<bool>(csv);
}

cv::Mat plotTrajectories(const std::vector<std::pair<Trajectory, cv::Scalar>>& trajectories,
                         const std::string& caption, int width, int height) {
    cv::Mat image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::Vec3d low(1e300, 1e300, 1e300), high(-1e300, -1e300, -1e300);
    for (const auto& entry : trajectories)
        for (const TimedPose& p : entry.first)
            for (int a = 0; a < 3; ++a) {
                low[a] = std::min(low[a], p.pose.t[a]);
                high[a] = std::max(high[a], p.pose.t[a]);
            }
    if (low[0] > high[0]) return image;
    int axes[3] = {0, 1, 2};
    std::sort(axes, axes + 3, [&](int x, int y) { return high[x] - low[x] > high[y] - low[y]; });
    const int u = std::min(axes[0], axes[1]), v = std::max(axes[0], axes[1]);
    const double span = std::max({high[u] - low[u], high[v] - low[v], 1e-6});
    const double margin = 50, scale = std::min(width, height) - 2 * margin;
    auto toPixel = [&](const cv::Vec3d& p) {
        return cv::Point(static_cast<int>(margin + (p[u] - low[u]) / span * scale),
                         static_cast<int>(height - margin - (p[v] - low[v]) / span * scale));
    };
    for (const auto& entry : trajectories)
        for (std::size_t i = 1; i < entry.first.size(); ++i)
            cv::line(image, toPixel(entry.first[i - 1].pose.t), toPixel(entry.first[i].pose.t), entry.second, 2,
                     cv::LINE_AA);
    const char* names = "xyz";
    cv::putText(image, caption + "  (" + names[u] + " right, " + names[v] + " up, span " + number(span, 2) + " m)",
                cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(30, 30, 30), 1, cv::LINE_AA);
    return image;
}

cv::Mat plotSeries(const std::vector<double>& x, const std::vector<double>& y, const std::string& caption,
                   const std::string& unit, int width, int height) {
    cv::Mat image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    if (x.size() < 2 || x.size() != y.size()) return image;
    const double x0 = x.front(), x1 = std::max(x.back(), x0 + 1e-9);
    const double y1 = std::max(*std::max_element(y.begin(), y.end()), 1e-9);
    const int margin = 40;
    auto toPixel = [&](double a, double b) {
        return cv::Point(static_cast<int>(margin + (a - x0) / (x1 - x0) * (width - 2 * margin)),
                         static_cast<int>(height - margin - b / y1 * (height - 2 * margin)));
    };
    cv::line(image, toPixel(x0, 0), toPixel(x1, 0), cv::Scalar(180, 180, 180), 1);
    for (std::size_t i = 1; i < x.size(); ++i)
        cv::line(image, toPixel(x[i - 1], y[i - 1]), toPixel(x[i], y[i]), cv::Scalar(40, 40, 200), 2, cv::LINE_AA);
    cv::putText(image, caption + " (max " + number(y1) + " " + unit + ")", cv::Point(10, 25),
                cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(30, 30, 30), 1, cv::LINE_AA);
    return image;
}

bool writeTrajectoryReport(const std::string& directory, const std::string& title,
                           const Trajectory& reference, const TrajectoryComparison& c, bool withScale) {
    if (!ensureDirectory(directory)) return false;
    std::ofstream md(fs::path(directory) / "report.md");
    md << "# " << title << "\n\n";
    md << "- Matched poses: " << c.matchedPoses << "\n";
    md << "- Alignment: " << (withScale ? "Sim(3), scale " + number(c.alignment.scale, 4) : std::string("SE(3)")) << "\n";
    md << "- ATE: " << formatStats(c.ate, "m") << "\n";
    md << "- RPE translation: " << formatStats(c.rpeTranslation, "m") << "\n";
    md << "- RPE rotation: " << formatStats(c.rpeRotation, "deg") << "\n\n";
    md << "![trajectory](trajectory.png)\n\n![ATE over time](ate.png)\n";

    std::ofstream csv(fs::path(directory) / "ate.csv");
    csv << "timestamp,ate_m\n" << std::setprecision(12);
    for (std::size_t i = 0; i < c.atePerPose.size(); ++i) csv << c.ateTimestamps[i] << ',' << c.atePerPose[i] << '\n';

    const cv::Mat plot = plotTrajectories({{reference, cv::Scalar(60, 60, 60)}, {c.alignedEstimate, cv::Scalar(40, 40, 220)}},
                                          "reference (grey) vs aligned estimate (red)");
    const cv::Mat series = plotSeries(c.ateTimestamps, c.atePerPose, "ATE over time", "m");
    return static_cast<bool>(md) && cv::imwrite((fs::path(directory) / "trajectory.png").string(), plot) &&
           cv::imwrite((fs::path(directory) / "ate.png").string(), series);
}

}  // namespace baekar
