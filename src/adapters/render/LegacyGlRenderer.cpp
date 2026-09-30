#include "adapters/render/LegacyGlRenderer.h"

#include "legacy_renderer.h"

namespace baekar {

LegacyGlRenderer::LegacyGlRenderer(int argc, char** argv) : argc_(argc), argv_(argv) {}

bool LegacyGlRenderer::initialize() {
    legacy_render::Initialize(&argc_, argv_);
    return true;
}

void LegacyGlRenderer::beginFrame() { legacy_render::BeginFrame(); }

void LegacyGlRenderer::drawBackground(const cv::Mat& bgr) { legacy_render::DrawBackground(bgr); }

void LegacyGlRenderer::drawOutlines(const std::vector<MarkerObservation>& markers) {
    std::vector<legacy_render::Outline> outlines;
    for (const MarkerObservation& marker : markers) {
        if (!marker.found) continue;
        legacy_render::Outline outline;
        outline.index = marker.markerIndex;
        outline.name = marker.name;
        outline.tracking = marker.tracking;
        outline.corners = marker.detectionCorners;
        outline.inliers = marker.inliers;
        outline.trackedPoints = marker.trackedPoints;
        outlines.push_back(outline);
    }
    legacy_render::DrawOutlines(outlines);
}

void LegacyGlRenderer::setProjection(const Mat4& projection) {
    legacy_render::SetProjection(projection.m);
}

void LegacyGlRenderer::drawMarkerAnchor(const MarkerObservation& marker, const Mat4& projection,
                                        const Pose& pose) {
    legacy_render::DrawMarkerAnchor(marker.detectionCorners, projection.m, pose.view.m);
}

void LegacyGlRenderer::drawHand(const HandState& hand) {
    legacy_render::DrawHand(hand.projection.m, hand.view.m);
}

void LegacyGlRenderer::endFrame() { legacy_render::EndFrame(); }

}  // namespace baekar
