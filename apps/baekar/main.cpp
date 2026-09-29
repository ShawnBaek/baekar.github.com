// Composition root: the only place that names concrete adapter types.

#include "adapters/frame_source/FrameSources.h"
#include "adapters/legacy/LegacyEnginePipeline.h"
#include "adapters/window/GlfwWindow.h"
#include "application/AppConfig.h"
#include "application/Application.h"

#ifdef __APPLE__
#include "platform/macos/AvFoundationFrameSource.h"
#endif

#include <cstdio>
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

    baekar::LegacyEnginePipeline pipeline(argc, argv);
    baekar::Application application(parsed.config, *window, *frames.source, pipeline);
    return application.run();
}
