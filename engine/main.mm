// BaekAR macOS host. Replaces the Win32 WinMain / D3D9 message pump
// from MarkerlessAR/EngineMain.cpp with GLFW + a Metal renderer + a
// modern C++ worker pipeline.

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>

#include "engine/render_metal/Renderer.h"
#include "engine/vision/BriskTracker.h"
#include "engine/vision/FeaturePose.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <thread>

namespace {

constexpr int kWidth = 640;
constexpr int kHeight = 480;
constexpr float kMarkerSizeMm = 100.0f;

struct SharedState {
    std::mutex                      mtx;
    std::condition_variable         cv;
    cv::Mat                         latestFrame;       // BGR
    cv::Mat                         latestGray;        // CV_8U
    std::atomic<bool>               quit{false};

    std::atomic<bool>               poseValid{false};
    std::array<cv::Point2f, 4>      corners{};
    cv::Vec3d                       rvec{}, tvec{};
};

void cameraThread(SharedState* s, cv::VideoCapture* cap) {
    while (!s->quit.load()) {
        cv::Mat frame;
        if (!cap->read(frame) || frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        {
            std::lock_guard<std::mutex> lk(s->mtx);
            s->latestFrame = frame;
            s->latestGray = gray;
        }
        s->cv.notify_all();
    }
}

void visionThread(SharedState* s, baekar::BriskTracker* tracker,
                  const baekar::CameraIntrinsics* cam) {
    cv::Mat prevGray;
    bool tracking = false;
    std::array<cv::Point2f, 4> corners{};
    int sinceRedetect = 0;

    while (!s->quit.load()) {
        cv::Mat frame, gray;
        {
            std::unique_lock<std::mutex> lk(s->mtx);
            s->cv.wait_for(lk, std::chrono::milliseconds(33),
                           [&]{ return !s->latestFrame.empty() || s->quit.load(); });
            if (s->quit.load()) break;
            frame = s->latestFrame.clone();
            gray  = s->latestGray.clone();
        }
        if (frame.empty()) continue;

        bool ok = false;
        if (tracking && !prevGray.empty()) {
            ok = tracker->track(prevGray, gray, corners);
            ++sinceRedetect;
        }
        if (!ok || sinceRedetect > 30) {
            ok = tracker->detectAndMatch(frame, corners);
            sinceRedetect = 0;
        }
        tracking = ok;
        prevGray = gray;

        if (ok) {
            cv::Vec3d r, t;
            if (baekar::solvePose(*cam, corners, kMarkerSizeMm, kMarkerSizeMm, r, t)) {
                std::lock_guard<std::mutex> lk(s->mtx);
                s->corners = corners;
                s->rvec = r;
                s->tvec = t;
                s->poseValid.store(true);
                continue;
            }
        }
        s->poseValid.store(false);
    }
}

[[noreturn]] void die(const std::string& msg) {
    std::fprintf(stderr, "baekar: %s\n", msg.c_str());
    std::exit(1);
}

} // namespace

int main(int argc, char** argv) {
    std::string referencePath = "MarkerlessAR/image/yejin.jpg";
    std::string calibPath     = "MarkerlessAR/calibration/calibration.txt";
    std::string offlineFrame;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--reference" && i + 1 < argc) referencePath = argv[++i];
        else if (a == "--calibration" && i + 1 < argc) calibPath = argv[++i];
        else if (a == "--offline" && i + 1 < argc) offlineFrame = argv[++i];
        else if (a == "--help") {
            std::printf("Usage: baekar [--reference IMG] [--calibration TXT] [--offline IMG]\n");
            return 0;
        }
    }

    cv::Mat reference = cv::imread(referencePath, cv::IMREAD_COLOR);
    if (reference.empty()) die("could not load reference image: " + referencePath);

    baekar::CameraIntrinsics cam;
    if (!baekar::loadCalibration(calibPath, cam, cv::Size(kWidth, kHeight))) {
        die("could not load calibration: " + calibPath);
    }

    baekar::BriskTracker tracker;
    if (!tracker.loadReference(reference)) die("BRISK reference indexing failed");

    if (!glfwInit()) die("glfwInit failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "BaekAR", nullptr, nullptr);
    if (!window) die("glfwCreateWindow failed");

    NSWindow* nsWin = glfwGetCocoaWindow(window);
    auto renderer = baekar::Renderer::Create((__bridge void*)nsWin, kWidth, kHeight);

    float proj[16];
    baekar::makeProjectionMatrix(cam, 1.0f, 10000.0f, proj);
    renderer->setProjection(proj);

    SharedState shared;
    cv::VideoCapture capture;
    std::thread camThread, visThread;

    if (offlineFrame.empty()) {
        if (!capture.open(0)) die("could not open default camera");
        capture.set(cv::CAP_PROP_FRAME_WIDTH, kWidth);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, kHeight);
        camThread = std::thread(cameraThread, &shared, &capture);
        visThread = std::thread(visionThread, &shared, &tracker, &cam);
    } else {
        cv::Mat still = cv::imread(offlineFrame, cv::IMREAD_COLOR);
        if (still.empty()) die("offline image missing: " + offlineFrame);
        cv::resize(still, still, cv::Size(kWidth, kHeight));
        cv::cvtColor(still, shared.latestGray, cv::COLOR_BGR2GRAY);
        shared.latestFrame = still;
        std::array<cv::Point2f, 4> corners;
        if (tracker.detectAndMatch(still, corners)) {
            cv::Vec3d r, t;
            if (baekar::solvePose(cam, corners, kMarkerSizeMm, kMarkerSizeMm, r, t)) {
                shared.rvec = r; shared.tvec = t;
                shared.corners = corners;
                shared.poseValid.store(true);
            }
        }
    }

    int framesRendered = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        cv::Mat frame;
        bool poseValid;
        cv::Vec3d rvec, tvec;
        {
            std::lock_guard<std::mutex> lk(shared.mtx);
            frame = shared.latestFrame;
            poseValid = shared.poseValid.load();
            rvec = shared.rvec;
            tvec = shared.tvec;
        }

        renderer->beginFrame();
        if (!frame.empty()) renderer->drawCameraBackground(frame);
        if (poseValid) {
            float view[16];
            baekar::makeViewMatrix(rvec, tvec, view);
            renderer->setView(view);
            renderer->drawAxis(50.0f);
        }
        renderer->endFrame();

        ++framesRendered;
        if (!offlineFrame.empty() && framesRendered >= 1) break;
    }

    shared.quit.store(true);
    shared.cv.notify_all();
    if (camThread.joinable()) camThread.join();
    if (visThread.joinable()) visThread.join();
    if (capture.isOpened()) capture.release();

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("baekar: rendered %d frame(s)\n", framesRendered);
    return 0;
}
