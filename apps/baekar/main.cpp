// Composition root: the only place that names concrete adapter types.

#include "adapters/frame_source/FrameSources.h"
#include "adapters/legacy/LegacyEnginePipeline.h"
#include "adapters/pose/LegacyCameraPoseEstimator.h"
#include "adapters/tracking/MarkerTrackers.h"
#include "adapters/window/GlfwWindow.h"
#include "application/AppConfig.h"
#include "application/Application.h"

#ifdef __APPLE__
#include "platform/macos/AvFoundationFrameSource.h"
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace {

// Data files (skin models, calibration/, 3dobjects/, image/) are symlinked
// next to the binary, so run from the executable's directory.
void changeToExecutableDirectory(const char* argv0) {
#ifndef _WIN32
    const std::string exePath(argv0);
    const auto lastSlash = exePath.rfind('/');
    if (lastSlash == std::string::npos) return;
    const std::string exeDir = exePath.substr(0, lastSlash);
    if (chdir(exeDir.c_str()) == 0)
        std::fprintf(stderr, "BaekAR: working directory set to %s\n", exeDir.c_str());
#else
    (void)argv0;
#endif
}

// Accepts a path, or a bare filename that lives in image/.
std::string resolveMarkerPath(const std::string& path) {
#ifndef _WIN32
    if (access(path.c_str(), R_OK) != 0 && path.find('/') == std::string::npos) {
        const std::string inImageDir = "image/" + path;
        if (access(inImageDir.c_str(), R_OK) == 0) return inImageDir;
    }
#endif
    return path;
}

// Marker-image picker (--interactive): the thesis "select a feature-
// detectable photo" workflow. Returns an empty string to keep the default.
std::string pickMarkerImage() {
    namespace fs = std::filesystem;
    std::vector<std::string> images;
    std::error_code error;
    for (const fs::directory_entry& entry : fs::directory_iterator("image", error)) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png")
            images.push_back(entry.path().filename().string());
    }
    std::sort(images.begin(), images.end());
    if (images.empty()) {
        std::fprintf(stderr, "MarkerPicker: no images found in 'image'\n");
        return "";
    }
    std::fprintf(stderr, "\n=== Pick a feature-detectable marker image ===\n");
    for (std::size_t i = 0; i < images.size(); ++i)
        std::fprintf(stderr, "  [%2zu] %s\n", i, images[i].c_str());
    std::fprintf(stderr, "Enter index (0-%zu), or anything else to keep default (yejin.jpg): ",
                 images.size() - 1);
    char line[64] = {0};
    if (!std::fgets(line, sizeof(line), stdin)) return "";
    int index = -1;
    if (std::sscanf(line, "%d", &index) != 1 || index < 0 || index >= static_cast<int>(images.size()))
        return "";
    return "image/" + images[static_cast<std::size_t>(index)];
}

// Marker images for the tracker: the simulated markers, --marker, the
// interactive picker, or the 2012 default.
std::vector<std::string> chooseMarkerImages(const baekar::AppConfig& config) {
    std::vector<std::string> markers;
    if (config.usesSimulation()) {
        for (const std::string& marker : config.simulatedMarkers) markers.push_back(resolveMarkerPath(marker));
        return markers;
    }
    std::string marker = config.markerImage;
    if (marker.empty() && config.interactive) marker = pickMarkerImage();
    markers.push_back(resolveMarkerPath(marker.empty() ? "image/yejin.jpg" : marker));
    std::fprintf(stderr, "BaekAR: marker = %s\n", markers.front().c_str());
    return markers;
}

// Simple factory: --tracker (auto picks the 2012 pipeline for one marker
// and the synchronized tracker for several).
std::unique_ptr<baekar::IMarkerTracker> makeMarkerTracker(baekar::TrackerKind kind, std::size_t markerCount) {
    const bool multi = kind == baekar::TrackerKind::Multi ||
                       (kind == baekar::TrackerKind::Auto && markerCount > 1);
    if (multi) return std::make_unique<baekar::MultiMarkerTracker>();
    return std::make_unique<baekar::LegacySingleMarkerTracker>();
}

struct FrameSourceSetup {
    std::unique_ptr<baekar::IFrameSource> source;
#ifdef __APPLE__
    baekar::AvFoundationFrameSource* camera = nullptr;  // for the Camera menu
#endif
};

// Simple factory: config -> opened IFrameSource. A camera that cannot be
// opened falls back to the placeholder source (Null Object), as the 2012
// engine did with its dummy frame.
FrameSourceSetup openFrameSource(const baekar::AppConfig& config) {
    FrameSourceSetup setup;
    bool isCamera = false;

    if (config.usesSimulation()) {
        std::vector<std::string> markers;
        for (const std::string& marker : config.simulatedMarkers)
            markers.push_back(resolveMarkerPath(marker));
        setup.source = std::make_unique<baekar::SyntheticFrameSource>(markers);
    } else if (!config.replayDirectory.empty()) {
        setup.source = std::make_unique<baekar::ImageSequenceFrameSource>(config.replayDirectory);
    } else {
        isCamera = true;
#ifdef __APPLE__
        int cameraIndex = config.cameraIndex;
        if (config.interactive && cameraIndex < 0) cameraIndex = baekar::pickCameraIndex();
        auto camera = std::make_unique<baekar::AvFoundationFrameSource>(cameraIndex);
        setup.camera = camera.get();
        setup.source = std::move(camera);
#else
        // The 2012 Capture class flipped non-AVFoundation camera frames vertically.
        setup.source = std::make_unique<baekar::OpenCvCameraFrameSource>(config.cameraIndex, true);
#endif
    }

    if (!config.recordDirectory.empty()) {
        setup.source = std::make_unique<baekar::RecordingFrameSource>(std::move(setup.source),
                                                                      config.recordDirectory);
    }

    if (setup.source->open()) return setup;

    if (!isCamera) {
        std::fprintf(stderr, "BaekAR: could not open %s\n", setup.source->describe().c_str());
        setup.source.reset();
        return setup;
    }
    std::fprintf(stderr, "BaekAR: camera unavailable — using the placeholder frame source.\n");
    setup.source = std::make_unique<baekar::DummyFrameSource>();
#ifdef __APPLE__
    setup.camera = nullptr;
#endif
    setup.source->open();
    return setup;
}

}  // namespace

int main(int argc, char* argv[]) {
    const baekar::ParseResult parsed = baekar::parseCommandLine(argc, argv);
    if (!parsed.ok) {
        std::fprintf(stderr, "BaekAR: %s\n\n%s", parsed.error.c_str(), baekar::usageText().c_str());
        return 2;
    }
    if (parsed.showHelp) {
        std::printf("%s", baekar::usageText().c_str());
        return 0;
    }

    changeToExecutableDirectory(argv[0]);

    auto window = baekar::GlfwWindow::create(640, 480, "BaekAR - Markerless AR Engine");
    if (!window) return 1;

    FrameSourceSetup frames = openFrameSource(parsed.config);
    if (!frames.source) return 1;
#ifdef __APPLE__
    // Menu-bar "Camera" menu; needs NSApp, which GLFW created with the window.
    if (frames.camera) baekar::installCameraMenu(*frames.camera, parsed.config.cameraIndex);
#endif

    const std::vector<std::string> markers = chooseMarkerImages(parsed.config);
    if (parsed.config.tracker == baekar::TrackerKind::Legacy && markers.size() != 1) {
        std::fprintf(stderr, "BaekAR: --tracker legacy follows exactly one marker.\n");
        return 2;
    }
    std::unique_ptr<baekar::IMarkerTracker> tracker = makeMarkerTracker(parsed.config.tracker, markers.size());
    baekar::LegacyCameraPoseEstimator poseEstimator;
    baekar::LegacyEnginePipeline pipeline(argc, argv);

    baekar::Dependencies dependencies{*window, *frames.source, *tracker, poseEstimator, pipeline};
    baekar::Application application(parsed.config, markers, "calibration/calibration.txt", dependencies);
    return application.run();
}
