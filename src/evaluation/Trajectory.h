#ifndef BAEKAR_EVALUATION_TRAJECTORY_H
#define BAEKAR_EVALUATION_TRAJECTORY_H

#include "core/Frame.h"
#include "core/Rotation.h"

#include <string>
#include <utility>
#include <vector>

namespace baekar {

struct TimedPose {
    double timestampSeconds = 0.0;
    RigidTransform pose;  // world_from_camera
};
using Trajectory = std::vector<TimedPose>;

// TUM trajectory text format: "timestamp tx ty tz qx qy qz qw" per line,
// '#' comments allowed.
bool readTumTrajectory(const std::string& path, Trajectory& out, std::string* error = nullptr);
bool writeTumTrajectory(const std::string& path, const Trajectory& trajectory);

// Pairs each pose of `a` with the closest-in-time pose of `b` within
// maxDifference seconds, each pose used at most once (TUM associate.py).
std::vector<std::pair<std::size_t, std::size_t>> associateByTime(const Trajectory& a,
                                                                 const Trajectory& b,
                                                                 double maxDifference);

}  // namespace baekar

#endif  // BAEKAR_EVALUATION_TRAJECTORY_H
