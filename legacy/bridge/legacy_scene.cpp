// Wraps Contents (was g_contents in EngineMain.cpp) and the window texture.
//
// Picking uses the same math as wonjo_dx::PickingRay (CalcPickingRay +
// TransformRay), but with the matrices the scene was actually drawn with
// and the 640x480 viewport. wonjo_dx::PickingRay read both from the D3D
// stub device, whose GetTransform/GetViewport do nothing outside Windows,
// so it divided by an uninitialized viewport size.

#include "legacy_scene.h"

#include "Contents.hpp"
#include "wonjo.h"

namespace legacy_scene {
namespace {

D3DXMATRIXA16 toD3DX(const Matrix& m) {
    D3DXMATRIXA16 out;
    for (int i = 0; i < 16; ++i) out[i] = m[i];
    return out;
}

const int kViewportWidth = 640;
const int kViewportHeight = 480;

}  // namespace

struct Scene::Impl {
    Contents contents;
    unsigned int windowTexture = 0;
    bool windowPlaneSpawned = false;
    D3DXMATRIXA16 projection;
    D3DXMATRIXA16 view;
    bool rendered = false;
};

Scene::Scene() : impl_(new Impl) {}

Scene::~Scene() = default;

void Scene::SetWindowTexture(const cv::Mat& bgra) {
    if (bgra.empty() || bgra.type() != CV_8UC4) return;
    wonjo_dx::UploadTexture(&impl_->windowTexture, bgra.data, bgra.cols, bgra.rows);
    if (impl_->windowTexture != 0 && !impl_->windowPlaneSpawned) {
        impl_->contents.addWindowPlane(impl_->windowTexture, /*halfSize=*/50.0f);
        impl_->windowPlaneSpawned = true;
    }
}

void Scene::Render(const Matrix& projection, const Matrix& view) {
    impl_->projection = toD3DX(projection);
    impl_->view = toD3DX(view);
    impl_->rendered = true;
    impl_->contents.render();
}

int Scene::Pick(double x, double y) const {
    if (!impl_->rendered) return -1;
    // CalcPickingRay: through the pixel, in view space.
    const D3DXMATRIXA16& proj = impl_->projection;
    const float px = (((2.0f * static_cast<float>(x)) / kViewportWidth) - 1.0f) / proj(0, 0);
    const float py = (((-2.0f * static_cast<float>(y)) / kViewportHeight) + 1.0f) / proj(1, 1);
    D3DXVECTOR3 origin(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 direction(px, py, 1.0f);

    // TransformRay: to world (marker) space with the inverse view.
    D3DXMATRIXA16 viewInverse;
    D3DXMatrixInverse(&viewInverse, 0, &impl_->view);
    D3DXVec3TransformCoord(&origin, &origin, &viewInverse);
    D3DXVec3TransformNormal(&direction, &direction, &viewInverse);
    D3DXVec3Normalize(&direction, &direction);
    return impl_->contents.pick(origin, direction);
}

void Scene::Select(int index) { impl_->contents.select(index); }

void Scene::DeselectAll() { impl_->contents.deselectAll(); }

void Scene::DragSelected(double dxPixels, double dyPixels) {
    impl_->contents.dragSelected(dxPixels, dyPixels);
}

int Scene::ItemCount() const { return static_cast<int>(impl_->contents.items().size()); }

int Scene::SelectedIndex() const { return impl_->contents.selectedIndex(); }

}  // namespace legacy_scene
