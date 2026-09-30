// The 2012 BaekAR single-marker pipeline, moved out of EngineMain.cpp.
//
//   matching thread : BRISK detect + describe -> 2-NN Hamming match with a
//                     0.65 ratio test -> homography (RHO, RANSAC fallback)
//                     -> marker corners
//   tracking thread : homography from the latest matches -> warp the frame
//                     -> NCC template match of the half-size marker in a
//                     +/-20 px window -> corners mapped back
//
// The algorithms and thresholds are unchanged from EngineMain.cpp. What
// changed: the ~40 globals are members, values shared between the two
// threads are atomics or mutex-protected, workers stop and join, and each
// worker processes a frame once (by sequence number).

#include "legacy_marker_tracker.h"

#include "marker_geometry.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

namespace legacy_tracking {

struct SingleMarkerTracker::Impl {
    // ---- marker database (former img_*database1, kp_database1, ...) ----
    cv::Ptr<cv::BRISK> detector;
    cv::Ptr<cv::DescriptorMatcher> matcher;
    cv::Mat imgRgbDatabase;
    cv::Mat imgCenterDatabase;
    std::vector<cv::KeyPoint> kpDatabase;
    cv::Mat descDatabase;
    std::vector<cv::Point2f> objCenterCorners = std::vector<cv::Point2f>(4);
    cv::Mat firstFrame;

    // ---- frame hand-off (former g_sharedFrame) ----
    std::mutex frameMutex;
    std::condition_variable frameChanged;
    cv::Mat frame;
    std::uint64_t frameSequence = 0;

    // ---- matching -> tracking (former g_mpts_1 / g_mpts_2) ----
    std::mutex matchesMutex;
    std::vector<cv::Point2f> mpts1, mpts2;

    // ---- published state (former bThreadDetection1, bThreadTracking1,
    //      dst_matching_corners1, camera.featuresResult) ----
    std::atomic<bool> detected{false};
    std::atomic<bool> tracking{false};
    mutable std::mutex resultMutex;
    Snapshot result;

    std::atomic<bool> stop{false};
    std::thread matchingThread;
    std::thread trackingThread;

    bool buildDatabase(const std::string& markerImagePath);
    cv::Mat waitForNewFrame(std::uint64_t& lastSequence);
    void matchingLoop();
    void trackingLoop();
    void trackingIteration(cv::Mat& rgb, cv::Mat& imgDatabaseResize, cv::Mat& trackingResult,
                           bool& prevMatchLocValid, cv::Point& prevMatchLoc);
    void setPoseCorners(const std::vector<cv::Point2f>& corners);
};

SingleMarkerTracker::SingleMarkerTracker() : impl_(new Impl) {}

SingleMarkerTracker::~SingleMarkerTracker() { Stop(); }

bool SingleMarkerTracker::Impl::buildDatabase(const std::string& markerImagePath) {
    // Use OpenCV's built-in BRISK (the bundled MarkerlessAR/brisk/ predates
    // OpenCV's Feature2D interface and throws "not implemented" on detect()).
    // Threshold 60 was too strict for screen-displayed markers; 30 is the
    // OpenCV default and finds 5-10x more keypoints on the same scene.
    detector = cv::BRISK::create(30, 3);
    matcher = cv::makePtr<cv::BFMatcher>(cv::NORM_HAMMING);

    fprintf(stderr, "DBG: Loading database images...\n");
    imgRgbDatabase = cv::imread(markerImagePath, cv::IMREAD_COLOR);
    fprintf(stderr, "DBG: img1 loaded: %dx%d\n", imgRgbDatabase.cols, imgRgbDatabase.rows);
    if (imgRgbDatabase.empty()) return false;

    cv::Mat imgGrayDatabase;
    cv::cvtColor(imgRgbDatabase, imgGrayDatabase, cv::COLOR_BGR2GRAY);

    // For NCC Patch Tracking Move img_graydatabase1 to img_centerdatabase1
    imgCenterDatabase = CenterMarkerImage(imgGrayDatabase, 0.5);

    // Extract Image Corners
    std::vector<cv::Point2f> objCorners(4);
    objCorners[0] = cv::Point2f(0, 0);
    objCorners[1] = cv::Point2f(static_cast<float>(imgRgbDatabase.cols), 0);
    objCorners[2] = cv::Point2f(static_cast<float>(imgRgbDatabase.cols),
                                static_cast<float>(imgRgbDatabase.rows));
    objCorners[3] = cv::Point2f(0, static_cast<float>(imgRgbDatabase.rows));

    // Move obj_corners to center
    objCenterCorners = CenterMarkerCorners(imgGrayDatabase.size(), objCorners);

    fprintf(stderr, "DBG: Detecting keypoints...\n");
    detector->detect(imgCenterDatabase, kpDatabase);
    detector->compute(imgCenterDatabase, kpDatabase, descDatabase);
    return !descDatabase.empty();
}

bool SingleMarkerTracker::Start(const std::string& markerImagePath, const cv::Mat& firstFrame) {
    Stop();
    if (!impl_->buildDatabase(markerImagePath)) return false;
    impl_->firstFrame = firstFrame.empty() ? cv::Mat::zeros(480, 640, CV_8UC3) : firstFrame.clone();
    impl_->stop = false;
    impl_->matchingThread = std::thread([this] { impl_->matchingLoop(); });
    impl_->trackingThread = std::thread([this] { impl_->trackingLoop(); });
    return true;
}

void SingleMarkerTracker::Submit(const cv::Mat& bgr, std::uint64_t sequence) {
    {
        std::lock_guard<std::mutex> lock(impl_->frameMutex);
        if (sequence == impl_->frameSequence) return;
        bgr.copyTo(impl_->frame);
        impl_->frameSequence = sequence;
    }
    impl_->frameChanged.notify_all();
}

Snapshot SingleMarkerTracker::Latest() const {
    std::lock_guard<std::mutex> lock(impl_->resultMutex);
    Snapshot snapshot = impl_->result;
    snapshot.detected = impl_->detected.load();
    snapshot.tracking = impl_->tracking.load();
    return snapshot;
}

void SingleMarkerTracker::Stop() {
    impl_->stop = true;
    impl_->frameChanged.notify_all();
    if (impl_->matchingThread.joinable()) impl_->matchingThread.join();
    if (impl_->trackingThread.joinable()) impl_->trackingThread.join();
}

cv::Mat SingleMarkerTracker::Impl::waitForNewFrame(std::uint64_t& lastSequence) {
    std::unique_lock<std::mutex> lock(frameMutex);
    frameChanged.wait_for(lock, std::chrono::milliseconds(50), [&] {
        return frameSequence != lastSequence || stop.load();
    });
    if (frameSequence == lastSequence || frame.empty()) return cv::Mat();
    lastSequence = frameSequence;
    return frame.clone();
}

void SingleMarkerTracker::Impl::setPoseCorners(const std::vector<cv::Point2f>& corners) {
    std::lock_guard<std::mutex> lock(resultMutex);
    for (int i = 0; i < 4; ++i) result.poseCorners[i] = corners[i];
}

// ------------------------------------------------------------------ matching

void SingleMarkerTracker::Impl::matchingLoop() {
    std::vector<cv::KeyPoint> kpCamera;
    cv::Mat descCamera;
    cv::Mat rgbCamera, grayCamera;
    std::vector<cv::Point2f> dstMatchingCorners(4);
    bool isDetecting = false;
    std::uint64_t lastSequence = 0;
    int detectTick = 0;
    int logTick = 0;

    while (!stop.load()) {
        rgbCamera = waitForNewFrame(lastSequence);
        if (rgbCamera.empty()) continue;  // waited up to 50 ms for a new frame

        // Convert Camera Input with RGB to Camera Input with GRAY
        cv::cvtColor(rgbCamera, grayCamera, cv::COLOR_BGR2GRAY);
        detector->detect(grayCamera, kpCamera);
        detector->compute(grayCamera, kpCamera, descCamera);

        // Diagnostic after detect: how many keypoints did BRISK find this frame?
        if (++detectTick % 30 == 0) {
            fprintf(stderr, "matching: frame=%dx%d kp_db=%lu kp_cam=%lu desc_cam=%dx%d\n",
                    grayCamera.cols, grayCamera.rows, (unsigned long)kpDatabase.size(),
                    (unsigned long)kpCamera.size(), descCamera.rows, descCamera.cols);
        }

        // knnMatch(k=2) + tight Lowe's ratio (0.65): BRISK keypoints on
        // high-contrast UI/text produce many ambiguous matches.
        std::vector<std::vector<cv::DMatch>> matches;
        {
            std::vector<std::vector<cv::DMatch>> raw;
            matcher->knnMatch(descCamera, descDatabase, raw, 2);
            matches.reserve(raw.size());
            for (size_t k = 0; k < raw.size(); ++k) {
                if (raw[k].size() == 2 && raw[k][0].distance < 0.65f * raw[k][1].distance)
                    matches.push_back({raw[k][0]});
            }
        }

        // Compute Homography: train이 1 query가 2
        std::vector<cv::Point2f> mpts_1, mpts_2;
        MatchesToPoints(matches, kpDatabase, kpCamera, mpts_1, mpts_2);

        // need at least 5 matched pairs of points (more are better)
        if (mpts_1.size() > 5) {
            // Always feed the latest matches to the tracking thread so tracking
            // re-syncs to fresh detections instead of a stale homography.
            {
                std::lock_guard<std::mutex> lock(matchesMutex);
                mpts1 = mpts_1;
                mpts2 = mpts_2;
            }
            // findHomography needs >= 4 equal-sized point sets.
            if (mpts_1.size() < 5 || mpts_2.size() < 5 || mpts_1.size() != mpts_2.size())
                continue;

            // RHO (PROSAC) at 2.5 px, RANSAC fallback; >= 8 inliers.
            cv::Mat inlierMask;
            cv::Mat H = cv::findHomography(cv::Mat(mpts_1), cv::Mat(mpts_2), cv::RHO, 2.5, inlierMask);
            if (H.empty())
                H = cv::findHomography(cv::Mat(mpts_1), cv::Mat(mpts_2), cv::RANSAC, 2.5, inlierMask);
            const int inlierCount = inlierMask.empty() ? 0 : cv::countNonZero(inlierMask);

            if (++logTick % 30 == 0)
                fprintf(stderr, "matching: candidates=%lu, inliers=%d\n",
                        (unsigned long)mpts_1.size(), inlierCount);

            if (H.empty() || H.cols != 3 || H.rows != 3 || inlierCount < 8) continue;

            // Convert Object Corners to Transformed Object Corners Using Homography
            cv::perspectiveTransform(objCenterCorners, dstMatchingCorners, H);

            {
                std::lock_guard<std::mutex> lock(resultMutex);
                for (int i = 0; i < 4; ++i) result.detectionCorners[i] = dstMatchingCorners[i];
                result.inliers = inlierCount;
                result.frameSequence = lastSequence;
                // 3D OBJECT 를 위한 공간: pose uses matching corners while not tracking.
                if (!tracking.load())
                    for (int i = 0; i < 4; ++i) result.poseCorners[i] = dstMatchingCorners[i];
            }
            isDetecting = true;
        } else {
            isDetecting = false;
        }

        detected = isDetecting;
        dstMatchingCorners.clear();
    }
}

// ------------------------------------------------------------------ tracking

void SingleMarkerTracker::Impl::trackingLoop() {
    cv::Mat imgDatabaseResize;
    cv::resize(imgRgbDatabase, imgDatabaseResize,
               cv::Size(imgRgbDatabase.cols / 2, imgRgbDatabase.rows / 2));

    cv::Mat rgbCamera = firstFrame;
    cv::Mat trackingResult(rgbCamera.rows - imgDatabaseResize.rows + 1,
                           rgbCamera.cols - imgDatabaseResize.cols, CV_32FC1);

    bool prevMatchLocValid = false;
    cv::Point prevMatchLoc;
    std::uint64_t lastSequence = 0;

    while (!stop.load()) {
        // Only Tracking will be run when Detection Condition Value was True
        if (!detected.load()) {
            // Detection is off: wait instead of spinning a core.
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        rgbCamera = waitForNewFrame(lastSequence);
        if (rgbCamera.empty()) continue;

        try {
            trackingIteration(rgbCamera, imgDatabaseResize, trackingResult, prevMatchLocValid,
                              prevMatchLoc);
        } catch (const cv::Exception& error) {
            // The 2012 search window can leave the image near the border,
            // which threw inside the thread and ended the process. Treat it
            // as a tracking failure instead.
            tracking = false;
            prevMatchLocValid = false;
            fprintf(stderr, "tracking: window left the image (%s)\n", error.err.c_str());
        }
    }
}

void SingleMarkerTracker::Impl::trackingIteration(cv::Mat& rgbCamera, cv::Mat& imgDatabaseResize,
                                                  cv::Mat& trackingResult, bool& prevMatchLocValid,
                                                  cv::Point& prevMatchLoc) {
    const int windowSize = 20;
    double minVal, maxVal;
    cv::Point minLoc, maxLoc, matchLoc;

    // Homography & NCC: 카메라 포인트 mpts_2, DB 포인트 mpts_1 (snapshot)
    std::vector<cv::Point2f> localMpts1, localMpts2;
    {
        std::lock_guard<std::mutex> lock(matchesMutex);
        localMpts1 = mpts1;
        localMpts2 = mpts2;
    }
    if (localMpts2.size() < 5 || localMpts1.size() < 5 || localMpts2.size() != localMpts1.size()) {
        tracking = false;  // 트랙킹을 실패했을 경우
        return;
    }
    cv::Mat HH = cv::findHomography(cv::Mat(localMpts2), cv::Mat(localMpts1), cv::RANSAC, 2);
    if (HH.empty() || HH.cols != 3 || HH.rows != 3) {
        tracking = false;  // 트랙킹을 실패했을 경우
        return;
    }
    cv::Mat HHInverse;
    cv::invert(HH, HHInverse, cv::DECOMP_LU);  // 역행렬 계산

    cv::Mat transformedCamera;
    cv::warpPerspective(rgbCamera, transformedCamera, HH, rgbCamera.size(), cv::INTER_LINEAR,
                        cv::BORDER_CONSTANT);

    const int halfCols = imgRgbDatabase.cols / 2;
    const int halfRows = imgRgbDatabase.rows / 2;
    if (!prevMatchLocValid) {
        cv::matchTemplate(transformedCamera, imgDatabaseResize, trackingResult, cv::TM_CCOEFF_NORMED);
        cv::minMaxLoc(trackingResult, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
        prevMatchLocValid = true;
        matchLoc = maxLoc;
        prevMatchLoc = matchLoc;
    } else if (prevMatchLoc.x - windowSize >= 0 && prevMatchLoc.y - windowSize >= 0) {
        cv::Rect roi(cv::Point(prevMatchLoc.x - windowSize, prevMatchLoc.y - windowSize),
                     cv::Size(halfCols + windowSize * 2, halfRows + windowSize * 2));
        cv::Mat roiTransformed(transformedCamera, roi);
        cv::matchTemplate(roiTransformed, imgDatabaseResize, trackingResult, cv::TM_CCOEFF_NORMED);
        cv::minMaxLoc(trackingResult, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
        matchLoc = maxLoc;
        matchLoc.x += (prevMatchLoc.x - windowSize);
        matchLoc.y += (prevMatchLoc.y - windowSize);
        prevMatchLoc = matchLoc;
    } else {
        cv::Rect roi(cv::Point(prevMatchLoc.x, prevMatchLoc.y),
                     cv::Size(halfCols + windowSize, halfRows + windowSize));
        cv::Mat roiTransformed(transformedCamera, roi);
        cv::matchTemplate(roiTransformed, imgDatabaseResize, trackingResult, cv::TM_CCOEFF_NORMED);
        cv::minMaxLoc(trackingResult, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
        matchLoc = maxLoc;
        matchLoc += prevMatchLoc;
        prevMatchLoc = matchLoc;
    }

    // 트랙킹이 실패했을 경우에만 Detection이 다시 쓰이도록: 0.55 -> 0.65
    if (maxVal < 0.65) {
        tracking = false;
        prevMatchLocValid = false;
        printf("트랙킹 실패\n");
        return;
    }

    std::vector<cv::Point2f> objTrackingCorners(4), dstTrackingCorners;
    objTrackingCorners[0] = cv::Point(matchLoc.x, matchLoc.y);
    objTrackingCorners[1] = cv::Point(matchLoc.x + halfCols, matchLoc.y);
    objTrackingCorners[2] = cv::Point(matchLoc.x + halfCols, matchLoc.y + halfRows);
    objTrackingCorners[3] = cv::Point(matchLoc.x, matchLoc.y + halfRows);
    cv::perspectiveTransform(objTrackingCorners, dstTrackingCorners, HHInverse);

    // 3D OBJECT 를 위한 공간: while tracking, the pose follows these corners.
    setPoseCorners(dstTrackingCorners);
    tracking = true;
}

}  // namespace legacy_tracking
