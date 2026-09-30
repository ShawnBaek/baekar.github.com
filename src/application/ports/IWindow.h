#ifndef BAEKAR_APPLICATION_PORTS_IWINDOW_H
#define BAEKAR_APPLICATION_PORTS_IWINDOW_H

#include "core/Input.h"

#include <opencv2/core.hpp>

namespace baekar {

// The window that owns the OpenGL context. All calls happen on the thread
// that created it (runtime invariant: GL stays on the GLFW context thread).
class IWindow {
public:
    virtual ~IWindow() = default;

    virtual bool shouldClose() const = 0;
    virtual void requestClose() = 0;
    virtual void pollEvents() = 0;
    virtual void swapBuffers() = 0;
    virtual PointerState pointer() const = 0;
    // Reads the back buffer that was just rendered, as BGR top-down.
    virtual cv::Mat readFramebuffer() const = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IWINDOW_H
