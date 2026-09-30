// Frame source strategies: synthetic -> record -> replay round trip, and the
// placeholder source used when no camera can be opened.

#include "adapters/frame_source/FrameSources.h"

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

    // Replay them: same count, same pixels (PNG is lossless).
    {
        baekar::ImageSequenceFrameSource replay(recordDir.string(), /*loop=*/false);
        expect(replay.open(), "replay opens");
        expect(replay.frameCount() == 20, "replay sees 20 frames");
        for (std::size_t i = 0; i < recorded.size(); ++i) {
            baekar::Frame frame;
            expect(replay.read(frame), "replay frame read");
            expect(cv::norm(frame.bgr, recorded[i], cv::NORM_INF) == 0.0, "replayed pixels match");
        }
        baekar::Frame extra;
        expect(!replay.read(extra), "non-looping replay ends");
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
