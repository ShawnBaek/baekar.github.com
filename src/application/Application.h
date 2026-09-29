#ifndef BAEKAR_APPLICATION_APPLICATION_H
#define BAEKAR_APPLICATION_APPLICATION_H

#include "application/AppConfig.h"
#include "application/ports/IEnginePipeline.h"
#include "application/ports/IWindow.h"

namespace baekar {

// Facade over the whole run: prepare, initialize, frame loop, shutdown.
// Replaces the InitializeEngineMain / mainLoop / ReleaseEngineMain trio.
class Application {
public:
    Application(AppConfig config, IWindow& window, IEnginePipeline& pipeline);

    // Returns the process exit code.
    int run();

    long renderedFrames() const { return renderedFrames_; }

private:
    bool saveScreenshot();

    AppConfig config_;
    IWindow& window_;
    IEnginePipeline& pipeline_;
    long renderedFrames_ = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_APPLICATION_H
