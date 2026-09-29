#ifndef BAEKAR_LEGACY_RENDERER_H
#define BAEKAR_LEGACY_RENDERER_H

// The 2012 renderer: wonjo_dx (D3D9-shaped API over fixed-function OpenGL)
// plus the overlays EngineMain.cpp drew. Plain C++ header for C++17 code.
// Every call must happen on the thread that owns the GL context.

#include <opencv2/core.hpp>

#include <array>
#include <string>
#include <vector>

namespace legacy_render {

using Matrix = std::array<float, 16>;  // 2012 D3DX layout

struct Outline {
    std::size_t index = 0;
    std::string name;
    bool tracking = false;
    std::array<cv::Point2f, 4> corners{};
    int inliers = 0;
    int trackedPoints = 0;
};

// GLUT, the D3D stub device and the 2012 GL state (former init()).
void Initialize(int* argc, char** argv);
void BeginFrame();
// The 2012 camera preview quad (flips the image as the 2012 loop did).
void DrawBackground(const cv::Mat& bgr);
// Colored outlines and labels (multi-marker tracker).
void DrawOutlines(const std::vector<Outline>& outlines);
void SetProjection(const Matrix& projection);
void SetView(const Matrix& view);
// The single-marker AR overlay: yellow rectangle and spinning Arrow3Axis.X
// in screen space, then the world-space plane and axis mesh.
void DrawMarkerAnchor(const std::array<cv::Point2f, 4>& corners, const Matrix& projection,
                      const Matrix& view);
// HandyAR's axis and hand meshes in the fingertip pose.
void DrawHand(const Matrix& projection, const Matrix& view);
void EndFrame();

}  // namespace legacy_render

#endif  // BAEKAR_LEGACY_RENDERER_H
