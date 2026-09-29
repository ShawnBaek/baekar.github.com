#ifndef BAEKAR_ADAPTERS_SCENE_CONTENTS_SCENE_H
#define BAEKAR_ADAPTERS_SCENE_CONTENTS_SCENE_H

#include "application/ports/IScene.h"

#include <memory>

namespace legacy_scene {
class Scene;
}

namespace baekar {

// Adapter over the 2012 Contents scene (window-texture planes).
class ContentsScene final : public IScene {
public:
    ContentsScene();
    ~ContentsScene() override;

    void setWindowTexture(const cv::Mat& bgra) override;
    void render(const Mat4& projection, const Mat4& view) override;
    int pick(double x, double y) const override;
    void select(int index) override;
    void deselectAll() override;
    void dragSelected(double dx, double dy) override;

private:
    std::unique_ptr<legacy_scene::Scene> scene_;
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_SCENE_CONTENTS_SCENE_H
