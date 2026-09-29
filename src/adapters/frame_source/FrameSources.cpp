#include "adapters/frame_source/FrameSources.h"

#include "synthetic_marker_source.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace baekar {

// ---------------------------------------------------------------- camera

OpenCvCameraFrameSource::OpenCvCameraFrameSource(int index, bool flipVertical)
    : index_(index < 0 ? 0 : index), flipVertical_(flipVertical) {}

bool OpenCvCameraFrameSource::open() {
    std::fprintf(stderr, "Capture: opening camera index %d...\n", index_);
    if (!capture_.open(index_)) {
        std::fprintf(stderr, "Capture: failed to open camera %d\n", index_);
        return false;
    }
    std::fprintf(stderr, "Capture: camera index=%d opened (native %dx%d, backend=%s)\n", index_,
                 static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH)),
                 static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT)),
                 capture_.getBackendName().c_str());
    capture_.set(cv::CAP_PROP_FRAME_WIDTH, kFrameWidth);
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, kFrameHeight);
    Frame first;
    return read(first);
}

bool OpenCvCameraFrameSource::read(Frame& frame) {
    cv::Mat image;
    if (!capture_.isOpened() || !capture_.read(image) || image.empty()) return false;
    cv::Mat sized = toEngineFrameSize(image);
    if (flipVertical_) cv::flip(sized, sized, 0);
    frame.bgr = sized.isContinuous() ? sized : sized.clone();
    frame.sequence = ++sequence_;
    frame.tickCount = cv::getTickCount();
    frame.placeholder = false;
    return true;
}

void OpenCvCameraFrameSource::close() {
    if (capture_.isOpened()) capture_.release();
}

std::string OpenCvCameraFrameSource::describe() const {
    return "camera " + std::to_string(index_) + " (OpenCV)";
}

// ------------------------------------------------------------- synthetic

SyntheticFrameSource::SyntheticFrameSource(std::vector<std::string> markerPaths)
    : markerPaths_(std::move(markerPaths)) {}

SyntheticFrameSource::~SyntheticFrameSource() = default;

bool SyntheticFrameSource::open() {
    source_ = std::make_unique<SyntheticMarkerSource>();
    if (!source_->Initialize(markerPaths_, kFrameWidth, kFrameHeight)) {
        source_.reset();
        return false;
    }
    return true;
}

bool SyntheticFrameSource::read(Frame& frame) {
    cv::Mat image;
    if (!source_ || !source_->NextFrame(image) || image.empty()) return false;
    frame.bgr = image;
    frame.sequence = ++sequence_;
    frame.tickCount = cv::getTickCount();
    frame.placeholder = false;
    return true;
}

void SyntheticFrameSource::close() { source_.reset(); }

std::string SyntheticFrameSource::describe() const {
    return "synthetic camera (" + std::to_string(markerPaths_.size()) + " marker(s))";
}

// ----------------------------------------------------------------- dummy

bool DummyFrameSource::open() {
    // Same image the 2012 engine drew when the camera could not be opened.
    image_ = cv::Mat::zeros(kFrameHeight, kFrameWidth, CV_8UC3);
    cv::putText(image_, "Camera Unavailable", cv::Point(140, 220), cv::FONT_HERSHEY_SIMPLEX, 1.2,
                cv::Scalar(0, 0, 255), 2);
    cv::putText(image_, "BaekAR Engine Running", cv::Point(130, 280), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                cv::Scalar(255, 255, 255), 2);
    return true;
}

bool DummyFrameSource::read(Frame& frame) {
    frame.bgr = image_;
    frame.sequence = 1;  // never changes: workers have nothing new to process
    frame.tickCount = cv::getTickCount();
    frame.placeholder = true;
    return true;
}

// -------------------------------------------------------------- replay

namespace {

bool isImageFile(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
}

}  // namespace

ImageSequenceFrameSource::ImageSequenceFrameSource(std::string directory, bool loop)
    : directory_(std::move(directory)), loop_(loop) {}

bool ImageSequenceFrameSource::open() {
    files_.clear();
    std::error_code error;
    for (const fs::directory_entry& entry : fs::directory_iterator(directory_, error)) {
        if (entry.is_regular_file() && isImageFile(entry.path()))
            files_.push_back(entry.path().string());
    }
    if (error) {
        std::fprintf(stderr, "Replay: cannot read %s: %s\n", directory_.c_str(), error.message().c_str());
        return false;
    }
    std::sort(files_.begin(), files_.end());
    next_ = 0;
    if (files_.empty()) {
        std::fprintf(stderr, "Replay: no images in %s\n", directory_.c_str());
        return false;
    }
    std::fprintf(stderr, "Replay: %zu frame(s) from %s\n", files_.size(), directory_.c_str());
    return true;
}

bool ImageSequenceFrameSource::read(Frame& frame) {
    if (files_.empty()) return false;
    if (next_ >= files_.size()) {
        if (!loop_) return false;
        next_ = 0;
    }
    cv::Mat image = cv::imread(files_[next_++], cv::IMREAD_COLOR);
    if (image.empty()) return false;
    frame.bgr = toEngineFrameSize(image);
    frame.sequence = ++sequence_;
    frame.tickCount = cv::getTickCount();
    frame.placeholder = false;
    return true;
}

std::string ImageSequenceFrameSource::describe() const { return "replay " + directory_; }

// ------------------------------------------------------------ recording

RecordingFrameSource::RecordingFrameSource(std::unique_ptr<IFrameSource> inner, std::string directory)
    : inner_(std::move(inner)), directory_(std::move(directory)) {}

bool RecordingFrameSource::open() {
    std::error_code error;
    fs::create_directories(directory_, error);
    if (error) {
        std::fprintf(stderr, "Record: cannot create %s: %s\n", directory_.c_str(), error.message().c_str());
        return false;
    }
    return inner_->open();
}

bool RecordingFrameSource::read(Frame& frame) {
    if (!inner_->read(frame)) return false;
    if (!frame.placeholder && frame.sequence != lastSequence_) {
        char name[32];
        std::snprintf(name, sizeof(name), "frame_%06zu.png", written_);
        if (cv::imwrite((fs::path(directory_) / name).string(), frame.bgr)) ++written_;
        lastSequence_ = frame.sequence;
    }
    return true;
}

void RecordingFrameSource::close() {
    inner_->close();
    std::fprintf(stderr, "Record: %zu frame(s) written to %s\n", written_, directory_.c_str());
}

std::string RecordingFrameSource::describe() const {
    return inner_->describe() + ", recording to " + directory_;
}

}  // namespace baekar
