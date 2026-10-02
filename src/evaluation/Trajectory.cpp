#include "evaluation/Trajectory.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace baekar {

bool readTumTrajectory(const std::string& path, Trajectory& out, std::string* error) {
    std::ifstream in(path);
    if (!in) {
        if (error) *error = "cannot open " + path;
        return false;
    }
    out.clear();
    std::string line;
    int lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        std::replace(line.begin(), line.end(), ',', ' ');
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        std::istringstream fields(line);
        double t, tx, ty, tz, qx, qy, qz, qw;
        if (!(fields >> t >> tx >> ty >> tz >> qx >> qy >> qz >> qw)) {
            if (error) *error = path + ":" + std::to_string(lineNumber) + ": expected 8 numbers";
            return false;
        }
        TimedPose p;
        p.timestampSeconds = t;
        p.pose.R = quaternionToRotation(qx, qy, qz, qw);
        p.pose.t = cv::Vec3d(tx, ty, tz);
        out.push_back(p);
    }
    std::sort(out.begin(), out.end(),
              [](const TimedPose& a, const TimedPose& b) { return a.timestampSeconds < b.timestampSeconds; });
    return true;
}

bool writeTumTrajectory(const std::string& path, const Trajectory& trajectory) {
    std::ofstream out(path);
    if (!out) return false;
    out << "# timestamp tx ty tz qx qy qz qw\n" << std::fixed << std::setprecision(9);
    for (const TimedPose& p : trajectory) {
        const cv::Vec4d q = rotationToQuaternion(p.pose.R);
        out << p.timestampSeconds << ' ' << p.pose.t[0] << ' ' << p.pose.t[1] << ' ' << p.pose.t[2] << ' '
            << q[0] << ' ' << q[1] << ' ' << q[2] << ' ' << q[3] << '\n';
    }
    return static_cast<bool>(out);
}

std::vector<std::pair<std::size_t, std::size_t>> associateByTime(const Trajectory& a,
                                                                 const Trajectory& b,
                                                                 double maxDifference) {
    struct Candidate {
        double difference;
        std::size_t i, j;
    };
    std::vector<Candidate> candidates;
    std::size_t start = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const double t = a[i].timestampSeconds;
        while (start < b.size() && b[start].timestampSeconds < t - maxDifference) ++start;
        for (std::size_t j = start; j < b.size() && b[j].timestampSeconds <= t + maxDifference; ++j)
            candidates.push_back({std::abs(b[j].timestampSeconds - t), i, j});
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& x, const Candidate& y) { return x.difference < y.difference; });
    std::vector<bool> usedA(a.size(), false), usedB(b.size(), false);
    std::vector<std::pair<std::size_t, std::size_t>> pairs;
    for (const Candidate& c : candidates) {
        if (usedA[c.i] || usedB[c.j]) continue;
        usedA[c.i] = usedB[c.j] = true;
        pairs.emplace_back(c.i, c.j);
    }
    std::sort(pairs.begin(), pairs.end());
    return pairs;
}

}  // namespace baekar
