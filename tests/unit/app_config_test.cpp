// Unit tests for the command-line parser that replaced the stdin pickers.

#include "application/AppConfig.h"

#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

baekar::ParseResult parse(std::initializer_list<const char*> args) {
    std::vector<const char*> argv{"BaekAR"};
    argv.insert(argv.end(), args.begin(), args.end());
    return baekar::parseCommandLine(static_cast<int>(argv.size()), argv.data());
}

}  // namespace

int main() {
    {
        const auto r = parse({});
        expect(r.ok && !r.showHelp, "no arguments are valid");
        expect(r.config.cameraIndex == -1, "default camera is platform default");
        expect(r.config.hand == baekar::HandKind::Auto, "hand tracker defaults to auto");
        expect(!r.config.interactive, "pickers are off by default");
        expect(r.config.maxFrames == 0, "runs until closed by default");
        expect(r.config.tracker == baekar::TrackerKind::Auto, "tracker defaults to auto");
    }
    {
        const auto r = parse({"--simulate-marker", "yejin.jpg", "--simulate-marker", "fish.jpg",
                              "--frames", "120", "--screenshot", "out.png", "--no-hand"});
        expect(r.ok, "simulation flags parse");
        expect(r.config.simulatedMarkers.size() == 2, "simulate-marker repeats");
        expect(r.config.usesSimulation(), "simulation detected");
        expect(r.config.maxFrames == 120, "frames parsed");
        expect(r.config.screenshotPath == "out.png", "screenshot parsed");
        expect(r.config.hand == baekar::HandKind::Off, "no-hand parsed");
    }
    {
        const auto r = parse({"--camera", "1", "--marker", "image/fish.jpg", "--tracker", "multi",
                              "--interactive", "--window-capture"});
        expect(r.ok, "camera flags parse");
        expect(r.config.cameraIndex == 1, "camera index parsed");
        expect(r.config.markerImage == "image/fish.jpg", "marker parsed");
        expect(r.config.tracker == baekar::TrackerKind::Multi, "tracker parsed");
        expect(r.config.interactive && r.config.windowCapture, "switches parsed");
    }
    {
        const auto r = parse({"--replay", "rec", "--record", "out"});
        expect(r.ok && r.config.replayDirectory == "rec" && r.config.recordDirectory == "out",
               "replay and record parsed");
    }
    expect(!parse({"--replay", "rec", "--simulate-marker", "a.jpg"}).ok,
           "replay and simulate-marker are exclusive");
    expect(parse({"--help"}).showHelp, "help flag");
    expect(!parse({"--frames"}).ok, "missing value rejected");
    expect(!parse({"--frames", "0"}).ok, "zero frames rejected");
    expect(!parse({"--frames", "12x"}).ok, "non-numeric frames rejected");
    expect(!parse({"--camera", "-2"}).ok, "negative camera rejected");
    expect(!parse({"--tracker", "fast"}).ok, "unknown tracker rejected");
    expect(parse({"--hand", "vision"}).config.hand == baekar::HandKind::Vision, "vision hand tracker parsed");
    expect(parse({"--hand", "handyar"}).config.hand == baekar::HandKind::HandyAr, "handyar hand tracker parsed");
    expect(parse({"--hand", "off"}).config.hand == baekar::HandKind::Off, "hand off parsed");
    expect(!parse({"--hand", "mediapipe"}).ok, "unknown hand tracker rejected");
    expect(!parse({"--hand"}).ok, "hand without a value rejected");
    expect(!parse({"--bogus"}).ok, "unknown option rejected");
    expect(!parse({"--tracker", "legacy", "--simulate-marker", "a.jpg",
                   "--simulate-marker", "b.jpg"}).ok,
           "legacy tracker with several markers rejected");
    expect(!parse({"--marker", "a.jpg", "--simulate-marker", "b.jpg"}).ok,
           "marker and simulate-marker are exclusive");

    if (failures == 0) std::fprintf(stderr, "app_config_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
