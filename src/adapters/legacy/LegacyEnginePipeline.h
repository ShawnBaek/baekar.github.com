#ifndef BAEKAR_ADAPTERS_LEGACY_LEGACY_ENGINE_PIPELINE_H
#define BAEKAR_ADAPTERS_LEGACY_LEGACY_ENGINE_PIPELINE_H

#include "application/ports/IEnginePipeline.h"

namespace baekar {

// Adapter: drives the 2012 engine (EngineMain.cpp) through legacy_engine.h.
class LegacyEnginePipeline final : public IEnginePipeline {
public:
    LegacyEnginePipeline(int argc, char** argv);

    bool prepare(const AppConfig& config) override;
    bool initializeRenderer() override;
    bool start(const Frame& firstFrame) override;
    void renderFrame(const Frame& frame, const PointerState& pointer) override;
    void shutdown() override;

private:
    int argc_;
    char** argv_;
    bool started_ = false;
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_LEGACY_LEGACY_ENGINE_PIPELINE_H
