#include "application/AppConfig.h"

#include <cerrno>
#include <cstdlib>

namespace baekar {
namespace {

bool parseLong(const std::string& text, long minimum, long& out) {
    if (text.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (errno != 0 || *end != '\0' || value < minimum) return false;
    out = value;
    return true;
}

}  // namespace

std::string usageText() {
    return
        "Usage: BaekAR [options]\n"
        "\n"
        "  --camera N               camera index (default: platform default)\n"
        "  --marker PATH            marker image for the camera pipeline\n"
        "  --simulate-marker PATH   use a synthetic camera; repeat for several markers\n"
        "  --tracker auto|legacy|multi\n"
        "                           marker tracker (auto: legacy for one marker,\n"
        "                           multi for several)\n"
        "  --no-hand                disable HandyAR fingertip tracking\n"
        "  --interactive            ask for camera, marker and window on stdin\n"
        "  --window-capture         render a captured window on the marker (macOS)\n"
        "  --frames N               quit after N rendered frames\n"
        "  --screenshot PATH        save the last rendered frame as an image\n"
        "  --help                   show this text\n";
}

ParseResult parseCommandLine(int argc, const char* const argv[]) {
    ParseResult result;
    AppConfig& config = result.config;

    auto fail = [&result](const std::string& message) {
        result.ok = false;
        result.error = message;
        return result;
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto needValue = [&](std::string& value) {
            if (i + 1 >= argc) return false;
            value = argv[++i];
            return true;
        };

        std::string value;
        if (arg == "--help" || arg == "-h") {
            result.ok = true;
            result.showHelp = true;
            return result;
        } else if (arg == "--camera") {
            long index = 0;
            if (!needValue(value) || !parseLong(value, 0, index))
                return fail("--camera needs a non-negative index");
            config.cameraIndex = static_cast<int>(index);
        } else if (arg == "--marker") {
            if (!needValue(value) || value.empty())
                return fail("--marker needs an image path");
            config.markerImage = value;
        } else if (arg == "--simulate-marker") {
            if (!needValue(value) || value.empty())
                return fail("--simulate-marker needs an image path or filename");
            config.simulatedMarkers.push_back(value);
        } else if (arg == "--tracker") {
            if (!needValue(value)) return fail("--tracker needs auto, legacy or multi");
            if (value == "auto") config.tracker = TrackerKind::Auto;
            else if (value == "legacy") config.tracker = TrackerKind::Legacy;
            else if (value == "multi") config.tracker = TrackerKind::Multi;
            else return fail("--tracker needs auto, legacy or multi");
        } else if (arg == "--no-hand") {
            config.handTracking = false;
        } else if (arg == "--interactive") {
            config.interactive = true;
        } else if (arg == "--window-capture") {
            config.windowCapture = true;
        } else if (arg == "--frames") {
            if (!needValue(value) || !parseLong(value, 1, config.maxFrames))
                return fail("--frames needs a positive number");
        } else if (arg == "--screenshot") {
            if (!needValue(value) || value.empty())
                return fail("--screenshot needs a file path");
            config.screenshotPath = value;
        } else {
            return fail("unknown option: " + arg);
        }
    }

    if (config.tracker == TrackerKind::Legacy && config.simulatedMarkers.size() > 1)
        return fail("--tracker legacy follows one marker; use multi for several");
    if (config.usesSimulation() && !config.markerImage.empty())
        return fail("--marker and --simulate-marker cannot be combined");

    result.ok = true;
    return result;
}

}  // namespace baekar
