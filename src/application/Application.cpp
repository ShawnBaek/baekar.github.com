#include "application/Application.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdio>
#include <utility>

namespace baekar {

Application::Application(AppConfig config, IWindow& window, IFrameSource& source,
                         IEnginePipeline& pipeline)
    : config_(std::move(config)), window_(window), source_(source), pipeline_(pipeline) {}

int Application::run() {
    if (!pipeline_.prepare(config_)) {
        std::fprintf(stderr, "BaekAR: platform preparation failed.\n");
        source_.close();
        return 1;
    }
    if (!pipeline_.initializeRenderer()) {
        std::fprintf(stderr, "BaekAR: renderer initialization failed.\n");
        source_.close();
        return 1;
    }
    std::fprintf(stderr, "BaekAR: frame source: %s\n", source_.describe().c_str());
    if (!source_.read(frame_)) {
        std::fprintf(stderr, "BaekAR: the frame source produced no first frame.\n");
        source_.close();
        return 1;
    }
    if (!pipeline_.start(frame_)) {
        std::fprintf(stderr, "BaekAR: engine start failed.\n");
        if (config_.usesSimulation())
            std::fprintf(stderr, "BaekAR: check marker paths and use visually distinct marker images.\n");
        pipeline_.shutdown();
        source_.close();
        return 1;
    }
    std::fprintf(stderr, "BaekAR: engine running. Press ESC to quit.\n");

    int exitCode = 0;
    while (!window_.shouldClose()) {
        // Keep the previous frame when the source has nothing new this tick.
        source_.read(frame_);
        pipeline_.renderFrame(frame_, window_.pointer());
        ++renderedFrames_;

        const bool lastFrame = config_.maxFrames > 0 && renderedFrames_ >= config_.maxFrames;
        if (lastFrame && !config_.screenshotPath.empty() && !saveScreenshot())
            exitCode = 1;

        window_.swapBuffers();
        window_.pollEvents();
        if (lastFrame) window_.requestClose();
    }

    pipeline_.shutdown();
    source_.close();
    std::fprintf(stderr, "BaekAR: rendered %ld frame(s).\n", renderedFrames_);
    return exitCode;
}

bool Application::saveScreenshot() {
    const cv::Mat image = window_.readFramebuffer();
    if (image.empty() || !cv::imwrite(config_.screenshotPath, image)) {
        std::fprintf(stderr, "BaekAR: could not write screenshot %s\n",
                     config_.screenshotPath.c_str());
        return false;
    }
    std::fprintf(stderr, "BaekAR: screenshot saved to %s\n", config_.screenshotPath.c_str());
    return true;
}

}  // namespace baekar
