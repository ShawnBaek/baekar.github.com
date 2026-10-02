#ifndef BAEKAR_EVALUATION_REPORT_H
#define BAEKAR_EVALUATION_REPORT_H

#include "evaluation/MarkerBenchmark.h"
#include "evaluation/Metrics.h"

#include <string>
#include <vector>

namespace baekar {

std::string formatStats(const ErrorStats& stats, const char* unit);

// Marker benchmark: report.md + samples.csv in `directory`.
bool writeMarkerReport(const std::string& directory, const std::string& title,
                       const MarkerBenchmarkResult& result, const std::vector<std::string>& markerNames,
                       const MarkerBenchmarkOptions& options);

// Trajectory comparison: report.md, ate.csv, trajectory.png (top view of the
// reference and the aligned estimate) and ate.png (error over time).
bool writeTrajectoryReport(const std::string& directory, const std::string& title,
                           const Trajectory& reference, const TrajectoryComparison& comparison,
                           bool withScale);

// Draws trajectories on the two axes with the largest extent.
cv::Mat plotTrajectories(const std::vector<std::pair<Trajectory, cv::Scalar>>& trajectories,
                         const std::string& caption, int width = 800, int height = 600);
cv::Mat plotSeries(const std::vector<double>& x, const std::vector<double>& y, const std::string& caption,
                   const std::string& unit, int width = 800, int height = 300);

}  // namespace baekar

#endif  // BAEKAR_EVALUATION_REPORT_H
