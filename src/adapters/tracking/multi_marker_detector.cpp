#include "multi_marker_detector.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <utility>

namespace {

const int kMinimumMatches = 8;
const int kTrackingRefreshInterval = 45;
const unsigned int kRecoveryIntervalFrames = 6;
const float kRatioThreshold = 0.65f;
const float kMaximumOpticalFlowError = 30.0f;
const float kMaximumForwardBackwardError = 1.5f;

std::string Basename(const std::string& path)
{
    const std::string::size_type separator = path.find_last_of("/\\");
    return separator == std::string::npos ? path : path.substr(separator + 1);
}

bool IsUsableQuadrilateral(const std::vector<cv::Point2f>& corners,
                           int frameWidth,
                           int frameHeight)
{
    if (corners.size() != 4 || !cv::isContourConvex(corners) ||
        std::fabs(cv::contourArea(corners)) < 250.0) {
        return false;
    }

    for (const cv::Point2f& corner : corners) {
        if (!std::isfinite(corner.x) || !std::isfinite(corner.y) ||
            corner.x < -frameWidth || corner.x > frameWidth * 2 ||
            corner.y < -frameHeight || corner.y > frameHeight * 2) {
            return false;
        }
    }
    return true;
}

}  // namespace

struct MultiMarkerDetector::Implementation {
    struct Worker {
        std::size_t markerIndex = 0;
        std::string markerName;
        cv::Mat referenceGray;
        std::vector<cv::KeyPoint> referenceKeypoints;
        cv::Mat referenceDescriptors;
        std::vector<cv::Point2f> referenceCorners;
        cv::BFMatcher matcher{cv::NORM_HAMMING};
        MultiMarkerDetection detection;
        std::vector<cv::Point2f> trackedReferencePoints;
        std::vector<cv::Point2f> trackedFramePoints;
        unsigned int trackingFramesSinceDetection = 0;
        std::vector<cv::Point2f> lastAcceptedCorners;
        std::uint64_t lastAcceptedFrameSequence = 0;
        std::vector<cv::Point2f> smoothedCorners;
        std::uint64_t smoothedFrameSequence = 0;
    };

    struct TrackSlice {
        std::size_t workerIndex = 0;
        std::size_t pointOffset = 0;
        std::size_t pointCount = 0;
    };

    mutable std::mutex frameMutex;
    mutable std::mutex detectionMutex;
    std::condition_variable frameAvailable;
    cv::Mat latestFrame;
    std::uint64_t latestFrameSequence = 0;
    cv::Ptr<cv::BRISK> brisk = cv::BRISK::create(30, 3);
    std::thread processingThread;
    bool running = false;
    std::vector<std::unique_ptr<Worker>> workers;

    bool ReferencesAreVisuallySimilar(const Worker& first,
                                      const Worker& second) const
    {
        if (first.referenceDescriptors.rows < 2 ||
            second.referenceDescriptors.rows < 2) {
            return false;
        }
        cv::BFMatcher matcher(cv::NORM_HAMMING);
        std::vector<std::vector<cv::DMatch>> nearestMatches;
        matcher.knnMatch(first.referenceDescriptors,
                         second.referenceDescriptors, nearestMatches, 2);

        std::vector<cv::Point2f> firstPoints;
        std::vector<cv::Point2f> secondPoints;
        for (const std::vector<cv::DMatch>& pair : nearestMatches) {
            if (pair.size() < 2 || pair[0].distance >= 0.70f * pair[1].distance) {
                continue;
            }
            firstPoints.push_back(first.referenceKeypoints[pair[0].queryIdx].pt);
            secondPoints.push_back(second.referenceKeypoints[pair[0].trainIdx].pt);
        }
        if (firstPoints.size() < 12) {
            return false;
        }

        cv::Mat inlierMask;
        cv::findHomography(firstPoints, secondPoints,
                           cv::RANSAC, 2.5, inlierMask);
        const int inlierCount = inlierMask.empty()
            ? 0 : cv::countNonZero(inlierMask);
        return inlierCount >= 10 &&
            inlierCount >= static_cast<int>(firstPoints.size() * 0.60);
    }

    void ResetTracking(Worker& worker)
    {
        worker.trackedReferencePoints.clear();
        worker.trackedFramePoints.clear();
        worker.trackingFramesSinceDetection = 0;
        worker.lastAcceptedCorners.clear();
        worker.lastAcceptedFrameSequence = 0;
    }

    void StabilizeCorners(Worker& worker, MultiMarkerDetection& result)
    {
        if (!result.found || result.corners.size() != 4) {
            return;
        }

        const bool needsReset = worker.smoothedCorners.size() != 4 ||
            result.frameSequence <= worker.smoothedFrameSequence ||
            result.frameSequence - worker.smoothedFrameSequence > 30;
        if (needsReset) {
            worker.smoothedCorners = result.corners;
            worker.smoothedFrameSequence = result.frameSequence;
            return;
        }

        float averageDistance = 0.0f;
        for (std::size_t index = 0; index < result.corners.size(); ++index) {
            averageDistance += cv::norm(result.corners[index] -
                                        worker.smoothedCorners[index]);
        }
        averageDistance /= static_cast<float>(result.corners.size());

        const float movementFactor = std::min(1.0f, averageDistance / 60.0f);
        const float baseAlpha = 0.18f + movementFactor * 0.52f;
        const std::uint64_t frameGap = std::min<std::uint64_t>(
            4, result.frameSequence - worker.smoothedFrameSequence);
        const float alpha = 1.0f - std::pow(1.0f - baseAlpha,
                                           static_cast<float>(frameGap));

        for (std::size_t index = 0; index < result.corners.size(); ++index) {
            const cv::Point2f delta = result.corners[index] -
                                      worker.smoothedCorners[index];
            if (cv::norm(delta) < 0.35f) {
                result.corners[index] = worker.smoothedCorners[index];
                continue;
            }
            worker.smoothedCorners[index] += delta * alpha;
            result.corners[index] = worker.smoothedCorners[index];
        }
        worker.smoothedFrameSequence = result.frameSequence;
    }

    bool QuadsAgree(const std::vector<cv::Point2f>& first,
                    const std::vector<cv::Point2f>& second,
                    float maximumAverageCornerDistance) const
    {
        if (first.size() != 4 || second.size() != 4) {
            return false;
        }

        const double firstArea = std::fabs(cv::contourArea(first));
        const double secondArea = std::fabs(cv::contourArea(second));
        if (firstArea < 250.0 || secondArea < 250.0) {
            return false;
        }
        const double areaRatio = secondArea / firstArea;
        if (areaRatio < 0.60 || areaRatio > 1.67) {
            return false;
        }

        float averageCornerDistance = 0.0f;
        for (std::size_t index = 0; index < first.size(); ++index) {
            averageCornerDistance += cv::norm(second[index] - first[index]);
        }
        return averageCornerDistance / 4.0f <= maximumAverageCornerDistance;
    }

    void SeedTrackingPoints(const cv::Mat& frameGray,
                            const cv::Mat& homography,
                            const std::vector<cv::Point2f>& markerCorners,
                            int referenceWidth,
                            int referenceHeight,
                            std::vector<cv::Point2f>& referencePoints,
                            std::vector<cv::Point2f>& framePoints) const
    {
        cv::Mat markerMask(frameGray.size(), CV_8UC1, cv::Scalar(0));
        std::vector<cv::Point> integerCorners;
        integerCorners.reserve(markerCorners.size());
        for (const cv::Point2f& corner : markerCorners) {
            integerCorners.push_back(cv::Point(cvRound(corner.x), cvRound(corner.y)));
        }
        cv::fillConvexPoly(markerMask, integerCorners, cv::Scalar(255), cv::LINE_AA);

        std::vector<cv::Point2f> seededFramePoints;
        cv::goodFeaturesToTrack(frameGray, seededFramePoints,
                                80, 0.01, 4.0, markerMask, 3, false, 0.04);
        if (seededFramePoints.size() < kMinimumMatches) {
            return;
        }

        std::vector<cv::Point2f> seededReferencePoints;
        cv::perspectiveTransform(seededFramePoints, seededReferencePoints,
                                 homography.inv());
        for (std::size_t index = 0; index < seededFramePoints.size(); ++index) {
            const cv::Point2f& referencePoint = seededReferencePoints[index];
            if (referencePoint.x < 0 || referencePoint.x >= referenceWidth ||
                referencePoint.y < 0 || referencePoint.y >= referenceHeight) {
                continue;
            }
            referencePoints.push_back(referencePoint);
            framePoints.push_back(seededFramePoints[index]);
        }
    }

    bool DetectMarker(Worker& worker,
                      const cv::Mat& frameGray,
                      const std::vector<cv::KeyPoint>& frameKeypoints,
                      const cv::Mat& frameDescriptors,
                      std::uint64_t frameSequence,
                      MultiMarkerDetection& result,
                      const std::vector<cv::Point2f>* expectedCorners)
    {
        if (worker.referenceDescriptors.empty() || frameDescriptors.empty()) {
            return false;
        }

        std::vector<std::vector<cv::DMatch>> nearestMatches;
        worker.matcher.knnMatch(worker.referenceDescriptors,
                                frameDescriptors, nearestMatches, 2);

        std::vector<cv::Point2f> referencePoints;
        std::vector<cv::Point2f> framePoints;
        for (const std::vector<cv::DMatch>& pair : nearestMatches) {
            if (pair.size() < 2 ||
                pair[0].distance >= kRatioThreshold * pair[1].distance) {
                continue;
            }
            referencePoints.push_back(
                worker.referenceKeypoints[pair[0].queryIdx].pt);
            framePoints.push_back(frameKeypoints[pair[0].trainIdx].pt);
        }

        result.candidateCount = static_cast<int>(referencePoints.size());
        if (referencePoints.size() < kMinimumMatches) {
            return false;
        }

        cv::Mat inlierMask;
        const cv::Mat homography = cv::findHomography(
            referencePoints, framePoints, cv::RANSAC, 2.5, inlierMask);
        result.inlierCount = inlierMask.empty() ? 0 : cv::countNonZero(inlierMask);
        if (homography.empty() || result.inlierCount < kMinimumMatches) {
            return false;
        }

        cv::perspectiveTransform(worker.referenceCorners,
                                 result.corners, homography);
        if (!IsUsableQuadrilateral(result.corners,
                                   frameGray.cols, frameGray.rows)) {
            result.corners.clear();
            return false;
        }
        if (expectedCorners &&
            !QuadsAgree(*expectedCorners, result.corners, 24.0f)) {
            result.corners.clear();
            return false;
        }

        std::vector<cv::Point2f> trackedReferencePoints;
        std::vector<cv::Point2f> trackedFramePoints;
        trackedReferencePoints.reserve(result.inlierCount);
        trackedFramePoints.reserve(result.inlierCount);
        const unsigned char* detectionInliers = inlierMask.ptr<unsigned char>();
        for (std::size_t index = 0; index < inlierMask.total(); ++index) {
            if (detectionInliers[index] == 0) {
                continue;
            }
            trackedReferencePoints.push_back(referencePoints[index]);
            trackedFramePoints.push_back(framePoints[index]);
        }

        std::vector<cv::Point2f> seededReferencePoints;
        std::vector<cv::Point2f> seededFramePoints;
        SeedTrackingPoints(frameGray, homography, result.corners,
                           worker.referenceGray.cols, worker.referenceGray.rows,
                           seededReferencePoints, seededFramePoints);
        if (seededFramePoints.size() >= kMinimumMatches) {
            trackedReferencePoints = std::move(seededReferencePoints);
            trackedFramePoints = std::move(seededFramePoints);
        }

        worker.trackedReferencePoints = std::move(trackedReferencePoints);
        worker.trackedFramePoints = std::move(trackedFramePoints);
        worker.trackingFramesSinceDetection = 0;
        worker.lastAcceptedCorners = result.corners;
        worker.lastAcceptedFrameSequence = frameSequence;
        result.frameSequence = frameSequence;
        result.trackedPointCount = static_cast<int>(worker.trackedFramePoints.size());
        result.usingOpticalFlow = false;
        result.found = true;
        return true;
    }

    void TrackMarkers(const cv::Mat& frameGray,
                      const std::vector<cv::Mat>& previousFramePyramid,
                      const std::vector<cv::Mat>& framePyramid,
                      std::uint64_t previousFrameSequence,
                      std::uint64_t frameSequence,
                      std::vector<MultiMarkerDetection>& results,
                      std::vector<unsigned char>& trackedWorkers)
    {
        if (previousFramePyramid.empty() || framePyramid.empty() ||
            previousFrameSequence == 0 || frameSequence <= previousFrameSequence ||
            frameSequence - previousFrameSequence > 12) {
            return;
        }

        std::vector<TrackSlice> slices;
        std::vector<cv::Point2f> previousPoints;
        for (std::size_t workerIndex = 0; workerIndex < workers.size(); ++workerIndex) {
            Worker& worker = *workers[workerIndex];
            if (worker.trackedFramePoints.size() < kMinimumMatches ||
                worker.trackedReferencePoints.size() != worker.trackedFramePoints.size() ||
                worker.lastAcceptedFrameSequence != previousFrameSequence) {
                continue;
            }

            TrackSlice slice;
            slice.workerIndex = workerIndex;
            slice.pointOffset = previousPoints.size();
            slice.pointCount = worker.trackedFramePoints.size();
            slices.push_back(slice);
            previousPoints.insert(previousPoints.end(),
                                  worker.trackedFramePoints.begin(),
                                  worker.trackedFramePoints.end());
        }
        if (previousPoints.empty()) {
            return;
        }

        std::vector<cv::Point2f> nextPoints;
        std::vector<unsigned char> forwardStatus;
        std::vector<float> forwardErrors;
        cv::calcOpticalFlowPyrLK(
            previousFramePyramid, framePyramid,
            previousPoints, nextPoints, forwardStatus, forwardErrors,
            cv::Size(21, 21), 3,
            cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS,
                             20, 0.03));

        std::vector<cv::Point2f> backwardPoints;
        std::vector<unsigned char> backwardStatus;
        std::vector<float> backwardErrors;
        cv::calcOpticalFlowPyrLK(
            framePyramid, previousFramePyramid,
            nextPoints, backwardPoints, backwardStatus, backwardErrors,
            cv::Size(21, 21), 3,
            cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS,
                             20, 0.03));

        for (const TrackSlice& slice : slices) {
            Worker& worker = *workers[slice.workerIndex];
            MultiMarkerDetection& result = results[slice.workerIndex];
            std::vector<cv::Point2f> referencePoints;
            std::vector<cv::Point2f> framePoints;
            referencePoints.reserve(slice.pointCount);
            framePoints.reserve(slice.pointCount);

            for (std::size_t localIndex = 0;
                 localIndex < slice.pointCount; ++localIndex) {
                const std::size_t index = slice.pointOffset + localIndex;
                if (!forwardStatus[index] || !backwardStatus[index] ||
                    forwardErrors[index] > kMaximumOpticalFlowError ||
                    backwardErrors[index] > kMaximumOpticalFlowError ||
                    cv::norm(backwardPoints[index] - previousPoints[index]) >
                        kMaximumForwardBackwardError ||
                    nextPoints[index].x < 0 ||
                    nextPoints[index].x >= frameGray.cols ||
                    nextPoints[index].y < 0 ||
                    nextPoints[index].y >= frameGray.rows) {
                    continue;
                }
                referencePoints.push_back(worker.trackedReferencePoints[localIndex]);
                framePoints.push_back(nextPoints[index]);
            }

            result.candidateCount = static_cast<int>(framePoints.size());
            if (framePoints.size() < kMinimumMatches) {
                continue;
            }

            cv::Mat inlierMask;
            const cv::Mat homography = cv::findHomography(
                referencePoints, framePoints, cv::RANSAC, 1.75, inlierMask);
            result.inlierCount = inlierMask.empty() ? 0 : cv::countNonZero(inlierMask);
            const int requiredInliers = std::max(
                kMinimumMatches,
                static_cast<int>(std::ceil(framePoints.size() * 0.65)));
            if (homography.empty() || result.inlierCount < requiredInliers) {
                continue;
            }

            cv::perspectiveTransform(worker.referenceCorners,
                                     result.corners, homography);
            if (!IsUsableQuadrilateral(result.corners,
                                       frameGray.cols, frameGray.rows)) {
                result.corners.clear();
                continue;
            }
            const std::uint64_t frameGap = frameSequence -
                                           worker.lastAcceptedFrameSequence;
            if (!QuadsAgree(worker.lastAcceptedCorners, result.corners,
                            10.0f + 8.0f * static_cast<float>(frameGap))) {
                result.corners.clear();
                continue;
            }

            std::vector<cv::Point2f> inlierReferencePoints;
            std::vector<cv::Point2f> inlierFramePoints;
            inlierReferencePoints.reserve(result.inlierCount);
            inlierFramePoints.reserve(result.inlierCount);
            const unsigned char* trackingInliers = inlierMask.ptr<unsigned char>();
            for (std::size_t index = 0; index < inlierMask.total(); ++index) {
                if (trackingInliers[index] == 0) {
                    continue;
                }
                inlierReferencePoints.push_back(referencePoints[index]);
                inlierFramePoints.push_back(framePoints[index]);
            }

            worker.trackedReferencePoints = std::move(inlierReferencePoints);
            worker.trackedFramePoints = std::move(inlierFramePoints);
            ++worker.trackingFramesSinceDetection;
            worker.lastAcceptedCorners = result.corners;
            worker.lastAcceptedFrameSequence = frameSequence;
            result.frameSequence = frameSequence;
            result.trackedPointCount = static_cast<int>(worker.trackedFramePoints.size());
            result.usingOpticalFlow = true;
            result.found = true;
            trackedWorkers[slice.workerIndex] = 1;
        }
    }

    void ProcessFrame(const cv::Mat& frameGray,
                      const std::vector<cv::Mat>& previousFramePyramid,
                      const std::vector<cv::Mat>& framePyramid,
                      std::uint64_t previousFrameSequence,
                      std::uint64_t frameSequence,
                      unsigned int& recoveryCooldown,
                      std::vector<MultiMarkerDetection>& results)
    {
        results.resize(workers.size());
        for (std::size_t index = 0; index < workers.size(); ++index) {
            results[index].markerIndex = workers[index]->markerIndex;
            results[index].markerName = workers[index]->markerName;
            results[index].frameSequence = frameSequence;
        }

        std::vector<unsigned char> trackedWorkers(workers.size(), 0);
        TrackMarkers(frameGray, previousFramePyramid, framePyramid,
                     previousFrameSequence, frameSequence,
                     results, trackedWorkers);

        bool hasLostWorker = false;
        bool needsRefresh = false;
        for (std::size_t index = 0; index < workers.size(); ++index) {
            Worker& worker = *workers[index];
            if (!trackedWorkers[index]) {
                ResetTracking(worker);
                hasLostWorker = true;
            } else if (worker.trackingFramesSinceDetection >=
                           kTrackingRefreshInterval ||
                       worker.trackedFramePoints.size() < kMinimumMatches + 2) {
                needsRefresh = true;
            }
        }

        if (!hasLostWorker) {
            recoveryCooldown = 0;
        } else if (recoveryCooldown > 0) {
            --recoveryCooldown;
        }
        const bool recoveryDue = hasLostWorker && recoveryCooldown == 0;
        const bool recoverLostWorkers = hasLostWorker &&
            (recoveryDue || needsRefresh);
        if (!needsRefresh && !recoveryDue) {
            return;
        }

        std::vector<cv::KeyPoint> frameKeypoints;
        cv::Mat frameDescriptors;
        brisk->detectAndCompute(frameGray, cv::noArray(),
                                frameKeypoints, frameDescriptors);
        if (recoverLostWorkers) {
            recoveryCooldown = kRecoveryIntervalFrames;
        }

        std::vector<std::size_t> detectionIndexes;
        for (std::size_t index = 0; index < workers.size(); ++index) {
            Worker& worker = *workers[index];
            const bool refreshWorker = trackedWorkers[index] &&
                (worker.trackingFramesSinceDetection >= kTrackingRefreshInterval ||
                 worker.trackedFramePoints.size() < kMinimumMatches + 2);
            if (refreshWorker ||
                (recoverLostWorkers && !trackedWorkers[index])) {
                detectionIndexes.push_back(index);
            }
        }

        std::vector<MultiMarkerDetection> detectionResults(workers.size());
        std::vector<unsigned char> detectionSucceeded(workers.size(), 0);
        cv::parallel_for_(cv::Range(0, static_cast<int>(detectionIndexes.size())),
                          [&](const cv::Range& range) {
            for (int detectionIndex = range.start;
                 detectionIndex < range.end; ++detectionIndex) {
                const std::size_t workerIndex =
                    detectionIndexes[static_cast<std::size_t>(detectionIndex)];
                Worker& worker = *workers[workerIndex];
                MultiMarkerDetection& detectionResult =
                    detectionResults[workerIndex];
                detectionResult.markerIndex = worker.markerIndex;
                detectionResult.markerName = worker.markerName;
                detectionResult.frameSequence = frameSequence;
                const std::vector<cv::Point2f>* expectedCorners =
                    trackedWorkers[workerIndex]
                    ? &results[workerIndex].corners : nullptr;
                detectionSucceeded[workerIndex] = DetectMarker(
                    worker, frameGray, frameKeypoints, frameDescriptors,
                    frameSequence, detectionResult, expectedCorners) ? 1 : 0;
            }
        });

        for (const std::size_t workerIndex : detectionIndexes) {
            Worker& worker = *workers[workerIndex];
            if (!detectionSucceeded[workerIndex]) {
                if (trackedWorkers[workerIndex]) {
                    worker.trackingFramesSinceDetection =
                        kTrackingRefreshInterval / 2;
                }
                continue;
            }

            if (trackedWorkers[workerIndex]) {
                // Refresh feature points without replacing the displayed pose.
                // This avoids a periodic BRISK-to-LK corner jump.
                worker.lastAcceptedCorners = results[workerIndex].corners;
                worker.lastAcceptedFrameSequence = frameSequence;
            } else {
                results[workerIndex] = std::move(detectionResults[workerIndex]);
            }
        }
    }

    void ProcessingLoop()
    {
        std::fprintf(stderr,
                     "BaekAR: synchronized detector started for %zu marker(s)\n",
                     workers.size());
        std::fflush(stderr);

        std::uint64_t processedFrameSequence = 0;
        std::uint64_t previousFrameSequence = 0;
        unsigned int recoveryCooldown = 0;
        unsigned int processedFrames = 0;
        std::vector<cv::Mat> previousFramePyramid;

        while (true) {
            cv::Mat frame;
            std::uint64_t frameSequence = 0;
            {
                std::unique_lock<std::mutex> lock(frameMutex);
                frameAvailable.wait(lock, [&] {
                    return !running || latestFrameSequence > processedFrameSequence;
                });
                if (!running) {
                    break;
                }
                frame = latestFrame;
                frameSequence = latestFrameSequence;
                processedFrameSequence = frameSequence;
            }

            std::vector<cv::Mat> framePyramid;
            cv::buildOpticalFlowPyramid(frame, framePyramid,
                                        cv::Size(21, 21), 3, true,
                                        cv::BORDER_REFLECT_101,
                                        cv::BORDER_CONSTANT, true);

            std::vector<MultiMarkerDetection> results;
            ProcessFrame(frame, previousFramePyramid, framePyramid,
                         previousFrameSequence, frameSequence,
                         recoveryCooldown, results);

            for (std::size_t index = 0; index < workers.size(); ++index) {
                StabilizeCorners(*workers[index], results[index]);
            }
            {
                std::lock_guard<std::mutex> lock(detectionMutex);
                for (std::size_t index = 0; index < workers.size(); ++index) {
                    workers[index]->detection = std::move(results[index]);
                }
            }

            previousFramePyramid = std::move(framePyramid);
            previousFrameSequence = frameSequence;

            if (++processedFrames % 60 == 0) {
                std::lock_guard<std::mutex> lock(detectionMutex);
                std::fprintf(stderr,
                             "BaekAR: synchronized tracking frame=%llu\n",
                             static_cast<unsigned long long>(frameSequence));
                for (const std::unique_ptr<Worker>& worker : workers) {
                    std::fprintf(stderr,
                                 "  %s mode=%s points=%d inliers=%d found=%s\n",
                                 worker->markerName.c_str(),
                                 worker->detection.usingOpticalFlow ? "track" : "detect",
                                 worker->detection.trackedPointCount,
                                 worker->detection.inlierCount,
                                 worker->detection.found ? "yes" : "no");
                }
                std::fflush(stderr);
            }
        }

        std::fprintf(stderr, "BaekAR: synchronized detector stopped\n");
        std::fflush(stderr);
    }
};

MultiMarkerDetector::MultiMarkerDetector()
    : implementation_(new Implementation())
{
}

MultiMarkerDetector::~MultiMarkerDetector()
{
    Stop();
}

bool MultiMarkerDetector::Initialize(const std::vector<std::string>& markerPaths)
{
    Stop();
    if (markerPaths.empty()) {
        return false;
    }

    for (std::size_t index = 0; index < markerPaths.size(); ++index) {
        std::unique_ptr<Implementation::Worker> worker(new Implementation::Worker());
        worker->markerIndex = index;
        worker->markerName = Basename(markerPaths[index]);
        worker->referenceGray = cv::imread(markerPaths[index], cv::IMREAD_GRAYSCALE);
        if (worker->referenceGray.empty()) {
            std::fprintf(stderr, "BaekAR: detector could not load %s\n",
                         markerPaths[index].c_str());
            implementation_->workers.clear();
            return false;
        }

        implementation_->brisk->detectAndCompute(
            worker->referenceGray, cv::noArray(),
            worker->referenceKeypoints, worker->referenceDescriptors);
        if (worker->referenceDescriptors.empty()) {
            std::fprintf(stderr, "BaekAR: detector found no BRISK features in %s\n",
                         markerPaths[index].c_str());
            implementation_->workers.clear();
            return false;
        }

        worker->referenceCorners = {
            cv::Point2f(0.0f, 0.0f),
            cv::Point2f(static_cast<float>(worker->referenceGray.cols - 1), 0.0f),
            cv::Point2f(static_cast<float>(worker->referenceGray.cols - 1),
                        static_cast<float>(worker->referenceGray.rows - 1)),
            cv::Point2f(0.0f, static_cast<float>(worker->referenceGray.rows - 1))
        };
        worker->detection.markerIndex = index;
        worker->detection.markerName = worker->markerName;
        implementation_->workers.push_back(std::move(worker));
    }

    for (std::size_t first = 0; first < implementation_->workers.size(); ++first) {
        for (std::size_t second = first + 1;
             second < implementation_->workers.size(); ++second) {
            if (!implementation_->ReferencesAreVisuallySimilar(
                    *implementation_->workers[first],
                    *implementation_->workers[second])) {
                continue;
            }
            std::fprintf(stderr,
                         "BaekAR: markers %s and %s are visually too similar "
                         "for unique tracking. Choose distinct images.\n",
                         implementation_->workers[first]->markerName.c_str(),
                         implementation_->workers[second]->markerName.c_str());
            implementation_->workers.clear();
            return false;
        }
    }

    {
        std::lock_guard<std::mutex> lock(implementation_->frameMutex);
        implementation_->latestFrame.release();
        implementation_->latestFrameSequence = 0;
        implementation_->running = true;
    }
    implementation_->processingThread = std::thread(
        &Implementation::ProcessingLoop, implementation_.get());
    return true;
}

void MultiMarkerDetector::SubmitFrame(const cv::Mat& bgrFrame)
{
    if (bgrFrame.empty()) {
        return;
    }

    cv::Mat frameGray;
    if (bgrFrame.channels() == 1) {
        frameGray = bgrFrame;
    } else {
        cv::cvtColor(bgrFrame, frameGray, cv::COLOR_BGR2GRAY);
    }
    cv::Mat immutableFrame = frameGray.clone();

    {
        std::lock_guard<std::mutex> lock(implementation_->frameMutex);
        if (!implementation_->running) {
            return;
        }
        implementation_->latestFrame = std::move(immutableFrame);
        ++implementation_->latestFrameSequence;
    }
    implementation_->frameAvailable.notify_one();
}

std::vector<MultiMarkerDetection> MultiMarkerDetector::LatestDetections() const
{
    std::lock_guard<std::mutex> lock(implementation_->detectionMutex);
    std::vector<MultiMarkerDetection> detections;
    detections.reserve(implementation_->workers.size());
    for (const std::unique_ptr<Implementation::Worker>& worker : implementation_->workers) {
        detections.push_back(worker->detection);
    }
    return detections;
}

std::size_t MultiMarkerDetector::WorkerCount() const
{
    return implementation_->workers.size();
}

void MultiMarkerDetector::Stop()
{
    {
        std::lock_guard<std::mutex> lock(implementation_->frameMutex);
        implementation_->running = false;
    }
    implementation_->frameAvailable.notify_all();

    if (implementation_->processingThread.joinable()) {
        implementation_->processingThread.join();
    }
    implementation_->workers.clear();
    implementation_->latestFrame.release();
    implementation_->latestFrameSequence = 0;
}
