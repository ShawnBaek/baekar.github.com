#ifndef BAEKAR_LEGACY_SCENE_H
#define BAEKAR_LEGACY_SCENE_H

// The 2012 AR content (Contents: window-texture planes in marker space)
// with picking. Plain C++ header for the C++17 layers.

#include <opencv2/core.hpp>

#include <array>
#include <memory>

namespace legacy_scene {

using Matrix = std::array<float, 16>;  // 2012 D3DX layout

class Scene {
public:
    Scene();
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Uploads a BGRA window frame and spawns the textured plane the first
    // time (GL thread only).
    void SetWindowTexture(const cv::Mat& bgra);
    // Draws every item with the marker pose and remembers the matrices for
    // picking.
    void Render(const Matrix& projection, const Matrix& view);
    // Returns the index of the item under the window pixel, or -1.
    int Pick(double x, double y) const;
    void Select(int index);
    void DeselectAll();
    void DragSelected(double dxPixels, double dyPixels);
    int ItemCount() const;
    int SelectedIndex() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace legacy_scene

#endif  // BAEKAR_LEGACY_SCENE_H
