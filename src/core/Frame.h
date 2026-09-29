#ifndef BAEKAR_CORE_FRAME_H
#define BAEKAR_CORE_FRAME_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <cstdint>

namespace baekar {

// The engine processes 640x480 BGR frames (the 2012 pipeline's size).
constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;

// One camera image. A source may hand out the same image twice (for example
// when AVFoundation has no fresh frame yet); the sequence number only
// increases for a new image.
struct Frame {
    cv::Mat bgr;                 // CV_8UC3, kFrameWidth x kFrameHeight
    std::uint64_t sequence = 0;  // 0 means "no frame yet"
    std::int64_t tickCount = 0;  // cv::getTickCount() when captured
    bool placeholder = false;    // true for "camera unavailable" frames
};

// Resizes to the engine frame size with the interpolation the 2012 Capture
// class used; returns the input unchanged if it already fits.
inline cv::Mat toEngineFrameSize(const cv::Mat& bgr) {
    if (bgr.cols == kFrameWidth && bgr.rows == kFrameHeight) return bgr;
    cv::Mat resized;
    cv::resize(bgr, resized, cv::Size(kFrameWidth, kFrameHeight), 0, 0, cv::INTER_AREA);
    return resized;
}

}  // namespace baekar

#endif  // BAEKAR_CORE_FRAME_H
