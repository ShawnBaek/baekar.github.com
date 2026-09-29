#include "adapters/scene/ContentsScene.h"

#include "legacy_scene.h"

namespace baekar {

ContentsScene::ContentsScene() : scene_(std::make_unique<legacy_scene::Scene>()) {}

ContentsScene::~ContentsScene() = default;

void ContentsScene::setWindowTexture(const cv::Mat& bgra) { scene_->SetWindowTexture(bgra); }

void ContentsScene::render(const Mat4& projection, const Mat4& view) {
    scene_->Render(projection.m, view.m);
}

int ContentsScene::pick(double x, double y) const { return scene_->Pick(x, y); }

void ContentsScene::select(int index) { scene_->Select(index); }

void ContentsScene::deselectAll() { scene_->DeselectAll(); }

void ContentsScene::dragSelected(double dx, double dy) { scene_->DragSelected(dx, dy); }

}  // namespace baekar
