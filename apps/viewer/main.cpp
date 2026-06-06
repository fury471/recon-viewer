#include <GLFW/glfw3.h>

int main() {
    glfwInit();

    // We'll render with Vulkan, not OpenGL, so tell GLFW not to
    // create an OpenGL context for this window.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Fixed size for now: a resizable window means rebuilding the
    // Vulkan swapchain on every resize, which we'll defer.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window =
        glfwCreateWindow(1280, 720, "recon-viewer", nullptr, nullptr);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}