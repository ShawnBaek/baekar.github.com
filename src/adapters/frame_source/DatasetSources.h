#ifndef BAEKAR_ADAPTERS_FRAME_SOURCE_DATASET_SOURCES_H
#define BAEKAR_ADAPTERS_FRAME_SOURCE_DATASET_SOURCES_H

// Recorded datasets as IFrameSource strategies (docs/dataset-format.md).

#include "application/ports/IFrameSource.h"

#include <memory>
#include <string>
#include <vector>

namespace baekar {

// One recorded image and what came with it, before loading pixels.
struct DatasetEntry {
    double timestampSeconds = 0.0;
    std::string rgbPath;
    std::string depthPath;  // empty: no depth
    bool hasPose = false;
    RigidTransform pose;    // world_from_camera
};

// Shared reader for list-of-entries formats (BaekAR, TUM, EuRoC).
class DatasetFrameSource : public IFrameSource {
public:
    bool read(Frame& frame) override;
    void close() override {}
    std::string describe() const override { return description_; }
    std::size_t frameCount() const { return entries_.size(); }
    const std::vector<DatasetEntry>& entries() const { return entries_; }

protected:
    explicit DatasetFrameSource(bool loop) : loop_(loop) {}
    bool finishOpen();  // sorts entries and IMU samples

    std::vector<DatasetEntry> entries_;
    std::vector<ImuSample> imu_;
    std::optional<CameraIntrinsics> intrinsics_;
    double depthUnitsPerMetre_ = 1000.0;
    bool colourIsGrey_ = false;
    std::string description_;

private:
    bool loop_;
    std::size_t next_ = 0;
    std::size_t nextImu_ = 0;
    std::uint64_t sequence_ = 0;
};

class BaekarDatasetFrameSource final : public DatasetFrameSource {
public:
    explicit BaekarDatasetFrameSource(std::string directory, bool loop = false);
    bool open() override;

private:
    std::string directory_;
};

class TumRgbdFrameSource final : public DatasetFrameSource {
public:
    explicit TumRgbdFrameSource(std::string directory, bool loop = false);
    bool open() override;

private:
    std::string directory_;
};

class EurocFrameSource final : public DatasetFrameSource {
public:
    explicit EurocFrameSource(std::string directory, bool loop = false);
    bool open() override;

private:
    std::string directory_;
};

// Decorator: resizes frames to the engine size (640x480) and scales the
// intrinsics, depth and reference marker corners with them.
class EngineSizeFrameSource final : public IFrameSource {
public:
    explicit EngineSizeFrameSource(std::unique_ptr<IFrameSource> inner);
    bool open() override { return inner_->open(); }
    bool read(Frame& frame) override;
    void close() override { inner_->close(); }
    std::string describe() const override { return inner_->describe(); }
    bool isLive() const override { return inner_->isLive(); }

private:
    std::unique_ptr<IFrameSource> inner_;
};

// Writes frames in the BaekAR dataset format.
class BaekarDatasetWriter {
public:
    bool open(const std::string& directory, const std::string& device);
    bool write(const Frame& frame);
    bool close();
    std::size_t framesWritten() const { return written_; }

private:
    std::string directory_;
    std::string device_;
    std::optional<CameraIntrinsics> intrinsics_;
    std::size_t written_ = 0;
    bool anyPose_ = false;
    std::vector<std::string> frameRows_;
    std::vector<std::string> imuRows_;
};

// Picks the reader from the folder contents: BaekAR, TUM RGB-D, EuRoC, or a
// plain folder of images. Returns nullptr if nothing matches.
std::unique_ptr<IFrameSource> openDatasetDirectory(const std::string& directory, bool loop);

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_FRAME_SOURCE_DATASET_SOURCES_H
