#ifndef BAEKAR_PLATFORM_MACOS_SCREEN_CAPTURE_WINDOW_SOURCE_H
#define BAEKAR_PLATFORM_MACOS_SCREEN_CAPTURE_WINDOW_SOURCE_H

#include "application/ports/IScene.h"

struct WCStream;

namespace baekar {

// Window pixels through ScreenCaptureKit (thesis "window as AR texture").
// open() asks for the window on stdin.
class ScreenCaptureWindowSource final : public IWindowTextureSource {
public:
    ~ScreenCaptureWindowSource() override;
    bool open() override;
    bool latest(cv::Mat& bgra) override;
    void close() override;

private:
    WCStream* stream_ = nullptr;
    cv::Mat buffer_;
};

}  // namespace baekar

#endif  // BAEKAR_PLATFORM_MACOS_SCREEN_CAPTURE_WINDOW_SOURCE_H
