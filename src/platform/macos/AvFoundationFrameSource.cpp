#include "platform/macos/AvFoundationFrameSource.h"

#include "compat/macos_av_capture.h"
#include "compat/macos_camera_auth.h"
#include "compat/macos_camera_menu.h"

#include <chrono>
#include <cstdio>
#include <thread>

namespace baekar {

AvFoundationFrameSource::AvFoundationFrameSource(int index) : index_(index) {}

AvFoundationFrameSource::~AvFoundationFrameSource() { close(); }

bool AvFoundationFrameSource::open() {
    std::fprintf(stderr, "BaekAR: requesting camera permission...\n");
    if (!RequestCameraPermission()) {
        std::fprintf(stderr, "BaekAR: camera permission denied — engine will run with dummy frames.\n");
        std::fprintf(stderr, "  Grant access in System Settings > Privacy & Security > Camera, then reset:\n");
        std::fprintf(stderr, "  tccutil reset Camera com.baekar.engine\n");
        return false;
    }
    std::fprintf(stderr, "BaekAR: camera permission granted.\n");
    if (!openDevice(index_)) return false;
    Frame first;
    return read(first);
}

bool AvFoundationFrameSource::openDevice(int index) {
    const int camIndex = index < 0 ? 0 : index;
    capture_ = AVCap_Open(camIndex, kFrameWidth, kFrameHeight);
    if (!capture_) {
        std::fprintf(stderr, "Capture: AVCap_Open failed for index %d\n", camIndex);
        return false;
    }
    int width = 0, height = 0;
    // Wait briefly for the first frame so the actual size is known.
    for (int i = 0; i < 50 && (width == 0 || height == 0); ++i) {
        AVCap_GetActualSize(capture_, &width, &height);
        if (width && height) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::fprintf(stderr, "Capture: AVCap opened index %d (native %dx%d)\n", camIndex, width, height);
    index_ = camIndex;
    return true;
}

bool AvFoundationFrameSource::read(Frame& frame) {
    if (!capture_) return false;
    int width = 0, height = 0;
    AVCap_GetActualSize(capture_, &width, &height);
    if (width == 0 || height == 0) return false;

    cv::Mat native(height, width, CV_8UC3);
    if (!AVCap_GetLatestBGR(capture_, native.data, width, height)) {
        // No fresh frame yet: hand out the previous one (same sequence).
        if (last_.sequence == 0) return false;
        frame = last_;
        return true;
    }
    last_.bgr = toEngineFrameSize(native);
    last_.sequence = ++sequence_;
    last_.tickCount = cv::getTickCount();
    last_.placeholder = false;
    frame = last_;
    return true;
}

void AvFoundationFrameSource::close() {
    if (capture_) {
        AVCap_Close(capture_);
        capture_ = nullptr;
    }
}

bool AvFoundationFrameSource::switchCamera(int index) {
    close();
    if (!openDevice(index)) return false;
    std::fprintf(stderr, "Capture: switched to camera %d\n", index);
    return true;
}

std::string AvFoundationFrameSource::describe() const {
    return "camera " + std::to_string(index_) + " (AVFoundation)";
}

namespace {
AvFoundationFrameSource* g_menuTarget = nullptr;
void onCameraMenuPicked(int index) {
    if (g_menuTarget) g_menuTarget->switchCamera(index);
}
}  // namespace

void installCameraMenu(AvFoundationFrameSource& source, int currentIndex) {
    g_menuTarget = &source;
    InstallCameraMenu(currentIndex < 0 ? 0 : currentIndex, &onCameraMenuPicked);
}

int pickCameraIndex() { return PickCameraIndex(); }

}  // namespace baekar
