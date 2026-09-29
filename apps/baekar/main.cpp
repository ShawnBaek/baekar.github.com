// Composition root: the only place that names concrete adapter types.

#include "adapters/legacy/LegacyEnginePipeline.h"
#include "adapters/window/GlfwWindow.h"
#include "application/AppConfig.h"
#include "application/Application.h"

#include <cstdio>
#include <string>

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

    baekar::LegacyEnginePipeline pipeline(argc, argv);
    baekar::Application application(parsed.config, *window, pipeline);
    return application.run();
}
