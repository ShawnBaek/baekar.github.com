#ifndef BAEKAR_APPLICATION_PORTS_ISCENE_H
#define BAEKAR_APPLICATION_PORTS_ISCENE_H

#include "core/Tracking.h"

#include <opencv2/core.hpp>

namespace baekar {

// AR content placed in marker space that the user can pick and drag.
class IScene {
public:
    virtual ~IScene() = default;

    // BGRA window pixels for the window-as-texture plane.
    virtual void setWindowTexture(const cv::Mat& bgra) = 0;
    virtual void render(const Mat4& projection, const Mat4& view) = 0;
    // Index of the item under a window pixel, or -1.
    virtual int pick(double x, double y) const = 0;
    virtual void select(int index) = 0;
    virtual void deselectAll() = 0;
    virtual void dragSelected(double dx, double dy) = 0;
};

// Source of window pixels for the scene (macOS ScreenCaptureKit).
class IWindowTextureSource {
public:
    virtual ~IWindowTextureSource() = default;
    virtual bool open() = 0;
    // Fills `bgra` when a new window frame is available.
    virtual bool latest(cv::Mat& bgra) = 0;
    virtual void close() = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_ISCENE_H
