#ifndef BAEKAR_ADAPTERS_FRAME_SOURCE_FRAME_SOURCES_H
#define BAEKAR_ADAPTERS_FRAME_SOURCE_FRAME_SOURCES_H

// Portable IFrameSource strategies. The macOS AVFoundation camera lives in
// src/platform/macos because it needs Apple frameworks.

#include "application/ports/IFrameSource.h"

#include <opencv2/videoio.hpp>

#include <memory>
#include <string>
#include <vector>

class SyntheticMarkerSource;

namespace baekar {

// Camera through cv::VideoCapture (V4L2 on Linux). Frames are resized to the
// engine size and, like the 2012 Capture class, optionally flipped
// vertically.
class OpenCvCameraFrameSource final : public IFrameSource {
public:
    OpenCvCameraFrameSource(int index, bool flipVertical);
    bool open() override;
    bool read(Frame& frame) override;
    void close() override;
    std::string describe() const override;

private:
    int index_;
    bool flipVertical_;
    cv::VideoCapture capture_;
    std::uint64_t sequence_ = 0;
};

// The moving virtual camera used by --simulate-marker.
class SyntheticFrameSource final : public IFrameSource {
public:
    explicit SyntheticFrameSource(std::vector<std::string> markerPaths);
    ~SyntheticFrameSource() override;
    bool open() override;
    bool read(Frame& frame) override;
    void close() override;
    std::string describe() const override;

private:
    std::vector<std::string> markerPaths_;
    std::unique_ptr<SyntheticMarkerSource> source_;
    std::uint64_t sequence_ = 0;
};

// Null Object used when no camera can be opened: a fixed
// "Camera Unavailable" frame, so rendering and UI keep running.
class DummyFrameSource final : public IFrameSource {
public:
    bool open() override;
    bool read(Frame& frame) override;
    void close() override {}
    std::string describe() const override { return "placeholder (camera unavailable)"; }
    bool isLive() const override { return false; }

private:
    cv::Mat image_;
};

// Replays frame_*.png / *.jpg images from a folder in name order, looping.
class ImageSequenceFrameSource final : public IFrameSource {
public:
    explicit ImageSequenceFrameSource(std::string directory, bool loop = true);
    bool open() override;
    bool read(Frame& frame) override;
    void close() override {}
    std::string describe() const override;
    std::size_t frameCount() const { return files_.size(); }

private:
    std::string directory_;
    bool loop_;
    std::vector<std::string> files_;
    std::size_t next_ = 0;
    std::uint64_t sequence_ = 0;
};

// Decorator: records every new frame of the wrapped source into a folder
// that ImageSequenceFrameSource can replay.
class RecordingFrameSource final : public IFrameSource {
public:
    RecordingFrameSource(std::unique_ptr<IFrameSource> inner, std::string directory);
    bool open() override;
    bool read(Frame& frame) override;
    void close() override;
    std::string describe() const override;
    bool isLive() const override { return inner_->isLive(); }
    std::size_t recordedFrames() const { return written_; }

private:
    std::unique_ptr<IFrameSource> inner_;
    std::string directory_;
    std::uint64_t lastSequence_ = 0;
    std::size_t written_ = 0;
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_FRAME_SOURCE_FRAME_SOURCES_H
