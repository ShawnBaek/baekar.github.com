// Frame source strategies: synthetic -> record -> replay round trip, and the
// placeholder source used when no camera can be opened.

#include "adapters/frame_source/FrameSources.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

}  // namespace

int main() {
    const std::string marker = std::string(BAEKAR_SOURCE_DIR) + "/assets/image/yejin.jpg";
    const fs::path recordDir = fs::temp_directory_path() / "baekar_frame_source_test";
    fs::remove_all(recordDir);

    // Record 20 synthetic frames through the decorator.
    std::vector<cv::Mat> recorded;
    {
        auto synthetic = std::make_unique<baekar::SyntheticFrameSource>(std::vector<std::string>{marker});
        baekar::RecordingFrameSource recorder(std::move(synthetic), recordDir.string());
        expect(recorder.open(), "recording synthetic source opens");
        std::uint64_t lastSequence = 0;
        for (int i = 0; i < 20; ++i) {
            baekar::Frame frame;
            expect(recorder.read(frame), "synthetic frame read");
            expect(frame.sequence > lastSequence, "sequence increases");
            expect(frame.bgr.cols == baekar::kFrameWidth && frame.bgr.rows == baekar::kFrameHeight,
                   "engine frame size");
            lastSequence = frame.sequence;
            recorded.push_back(frame.bgr.clone());
        }
        recorder.close();
        expect(recorder.recordedFrames() == 20, "20 frames written");
    }

    // Replay them as a BaekAR dataset: same count, same pixels (PNG is
    // lossless), and the intrinsics and timestamps the source provided.
    {
        auto replay = baekar::openDatasetDirectory(recordDir.string(), /*loop=*/false);
        expect(dynamic_cast<baekar::BaekarDatasetFrameSource*>(replay.get()) != nullptr,
               "--record writes a BaekAR dataset");
        expect(replay && replay->open(), "replay opens");
        std::size_t index = 0;
        baekar::Frame frame;
        while (replay && replay->read(frame)) {
            expect(index < recorded.size() && cv::norm(frame.bgr, recorded[index], cv::NORM_INF) == 0.0,
                   "replayed pixels match");
            expect(frame.intrinsics.has_value() && frame.intrinsics->width == baekar::kFrameWidth,
                   "intrinsics recorded");
            expect(std::abs(frame.timestampSeconds - index / 30.0) < 1e-6, "timestamps recorded");
            ++index;
        }
        expect(index == 20, "replay sees 20 frames");
    }

    // Placeholder source: never live, never a new sequence.
    {
        baekar::DummyFrameSource dummy;
        expect(dummy.open() && !dummy.isLive(), "dummy opens and is not live");
        baekar::Frame a, b;
        expect(dummy.read(a) && dummy.read(b), "dummy reads");
        expect(a.placeholder && a.sequence == b.sequence, "dummy frames are placeholders");
    }

    fs::remove_all(recordDir);
    if (failures == 0) std::fprintf(stderr, "frame_source_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
