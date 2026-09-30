#ifndef BAEKAR_ADAPTERS_WINDOW_GLFW_WINDOW_H
#define BAEKAR_ADAPTERS_WINDOW_GLFW_WINDOW_H

#include "application/ports/IWindow.h"

#include <memory>
#include <string>

struct GLFWwindow;

namespace baekar {

// RAII owner of glfwInit/glfwTerminate and one legacy (2.1) GL window.
class GlfwWindow final : public IWindow {
public:
    static std::unique_ptr<GlfwWindow> create(int width, int height, const std::string& title);
    ~GlfwWindow() override;

    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    bool shouldClose() const override;
    void requestClose() override;
    void pollEvents() override;
    void swapBuffers() override;
    PointerState pointer() const override;
    cv::Mat readFramebuffer() const override;

    GLFWwindow* handle() const { return window_; }

private:
    explicit GlfwWindow(GLFWwindow* window);
    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onMouseButton(GLFWwindow* window, int button, int action, int mods);

    GLFWwindow* window_ = nullptr;
    bool leftDown_ = false;
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_WINDOW_GLFW_WINDOW_H
