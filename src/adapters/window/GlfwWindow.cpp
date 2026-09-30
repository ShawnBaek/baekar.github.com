#include "adapters/window/GlfwWindow.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <opencv2/core.hpp>

#include <cstdio>

namespace baekar {

std::unique_ptr<GlfwWindow> GlfwWindow::create(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        std::fprintf(stderr, "BaekAR: glfwInit failed\n");
        return nullptr;
    }
    // Legacy profile: the 2012 renderer uses fixed-function OpenGL.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "BaekAR: glfwCreateWindow failed\n");
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::unique_ptr<GlfwWindow> result(new GlfwWindow(window));
    glfwSetWindowUserPointer(window, result.get());
    glfwSetKeyCallback(window, &GlfwWindow::onKey);
    glfwSetMouseButtonCallback(window, &GlfwWindow::onMouseButton);
    return result;
}

GlfwWindow::GlfwWindow(GLFWwindow* window) : window_(window) {}

GlfwWindow::~GlfwWindow() {
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

bool GlfwWindow::shouldClose() const { return glfwWindowShouldClose(window_) != 0; }

void GlfwWindow::requestClose() { glfwSetWindowShouldClose(window_, GLFW_TRUE); }

void GlfwWindow::pollEvents() { glfwPollEvents(); }

void GlfwWindow::swapBuffers() { glfwSwapBuffers(window_); }

PointerState GlfwWindow::pointer() const {
    PointerState state;
    glfwGetCursorPos(window_, &state.x, &state.y);
    state.leftDown = leftDown_;
    return state;
}

cv::Mat GlfwWindow::readFramebuffer() const {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    if (width <= 0 || height <= 0) return cv::Mat();

    cv::Mat bottomUp(height, width, CV_8UC3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, bottomUp.data);
    cv::Mat topDown;
    cv::flip(bottomUp, topDown, 0);
    return topDown;
}

void GlfwWindow::onKey(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void GlfwWindow::onMouseButton(GLFWwindow* window, int button, int action, int) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
    if (self && button == GLFW_MOUSE_BUTTON_LEFT)
        self->leftDown_ = (action == GLFW_PRESS);
}

}  // namespace baekar
