#ifndef BAEKAR_ADAPTERS_RENDER_LEGACY_GL_RENDERER_H
#define BAEKAR_ADAPTERS_RENDER_LEGACY_GL_RENDERER_H

#include "application/ports/IRenderer.h"

namespace baekar {

// Adapter over the 2012 renderer (wonjo_dx over fixed-function OpenGL).
class LegacyGlRenderer final : public IRenderer {
public:
    // argc/argv are forwarded to glutInit.
    LegacyGlRenderer(int argc, char** argv);

    bool initialize() override;
    void beginFrame() override;
    void drawBackground(const cv::Mat& bgr) override;
    void drawOutlines(const std::vector<MarkerObservation>& markers) override;
    void setProjection(const Mat4& projection) override;
    void drawMarkerAnchor(const MarkerObservation& marker, const Mat4& projection,
                          const Pose& pose) override;
    void drawHand(const HandState& hand) override;
    void endFrame() override;

private:
    int argc_;
    char** argv_;
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_RENDER_LEGACY_GL_RENDERER_H
