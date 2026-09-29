#ifndef BAEKAR_APPLICATION_APP_CONFIG_H
#define BAEKAR_APPLICATION_APP_CONFIG_H

#include <string>
#include <vector>

namespace baekar {

enum class TrackerKind { Auto, Legacy, Multi };

// Every startup choice BaekAR makes. Replaces the stdin pickers and the
// g_markerSimulation* / Filename[] globals that main() used to fill in.
struct AppConfig {
    int cameraIndex = -1;                       // -1: platform default
    std::string markerImage;                    // empty: image/yejin.jpg
    std::vector<std::string> simulatedMarkers;  // non-empty: synthetic camera
    TrackerKind tracker = TrackerKind::Auto;
    bool handTracking = true;
    bool interactive = false;    // run the stdin camera/marker/window pickers
    bool windowCapture = false;  // macOS ScreenCaptureKit window-as-texture
    long maxFrames = 0;          // 0: run until the window closes
    std::string screenshotPath;  // written after the last rendered frame

    bool usesSimulation() const { return !simulatedMarkers.empty(); }
};

struct ParseResult {
    bool ok = false;
    bool showHelp = false;
    std::string error;
    AppConfig config;
};

ParseResult parseCommandLine(int argc, const char* const argv[]);
std::string usageText();

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_APP_CONFIG_H
