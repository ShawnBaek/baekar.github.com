#pragma once

#include <memory>
#include <opencv2/core.hpp>

namespace baekar {

class Renderer {
public:
    // nsWindowHandle is a void* wrapping the Cocoa NSWindow* (so callers
    // don't need to drag Cocoa types into C++ headers).
    static std::unique_ptr<Renderer> Create(void* nsWindowHandle, int width, int height);

    virtual ~Renderer() = default;

    virtual void beginFrame() = 0;
    // Camera frame in OpenCV BGR8 layout, any size; the renderer rescales.
    virtual void drawCameraBackground(const cv::Mat& bgr) = 0;
    virtual void setProjection(const float colMajor[16]) = 0;
    virtual void setView(const float colMajor[16]) = 0;
    // Draws an X/Y/Z axis triad (red/green/blue) at the world origin.
    virtual void drawAxis(float length) = 0;
    virtual void endFrame() = 0;
};

} // namespace baekar
