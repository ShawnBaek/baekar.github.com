#include "platform/macos/ScreenCaptureWindowSource.h"

#include "platform/macos/macos_window_capture.h"

#include <cstdio>

namespace baekar {
namespace {
const int kTextureSize = 512;
}

ScreenCaptureWindowSource::~ScreenCaptureWindowSource() { close(); }

bool ScreenCaptureWindowSource::open() {
    const uint32_t windowId = WCPicker_PickWindowID();
    if (windowId == 0) {
        std::fprintf(stderr, "BaekAR: no window chosen — running marker-only mode.\n");
        return false;
    }
    stream_ = WCStream_Open(windowId, kTextureSize, kTextureSize);
    if (!stream_) return false;
    buffer_.create(kTextureSize, kTextureSize, CV_8UC4);
    std::fprintf(stderr, "BaekAR: window stream ready — plane will spawn on the marker.\n");
    return true;
}

bool ScreenCaptureWindowSource::latest(cv::Mat& bgra) {
    if (!stream_ || !WCStream_LatestFrame(stream_, buffer_.data, kTextureSize, kTextureSize))
        return false;
    bgra = buffer_;
    return true;
}

void ScreenCaptureWindowSource::close() {
    if (stream_) {
        WCStream_Close(stream_);
        stream_ = nullptr;
    }
}

}  // namespace baekar
