#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "gpu/Context.h"
#include "render/Swapchain.h"
#include "render/Renderer.h"
#include "render/PointRenderable.h"
#include "render/OrbitCamera.h"
#include "core/math/Transforms.h"

#include <spdlog/spdlog.h>

#include <cstdint>

int main() {
    if (!glfwInit()) { spdlog::error("GLFW init failed"); return 1; }
    if (!glfwVulkanSupported()) {
        spdlog::error("Vulkan not supported on this system");
        glfwTerminate();
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window =
        glfwCreateWindow(1280, 720, "recon-viewer", nullptr, nullptr);

    {   // context lives in this scope so it tears down before the window
        uint32_t count = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&count);

        gpu::ContextCreateInfo info;
        info.requiredInstanceExtensions.assign(glfwExts, glfwExts + count);
        info.createSurface = [window](VkInstance instance) -> VkSurfaceKHR {
            VkSurfaceKHR surface = VK_NULL_HANDLE;
            if (glfwCreateWindowSurface(instance, window, nullptr, &surface)
                != VK_SUCCESS)
                return VK_NULL_HANDLE;
            return surface;
            };
#ifdef NDEBUG
        info.enableValidation = false;   // release: no validation overhead
#else
        info.enableValidation = true;    // debug: validation layers on
#endif

        gpu::Context context(info);
        spdlog::info("Rendering on: {}", context.deviceName());

        render::Swapchain swapchain(context, 1280, 720);
        render::PointRenderable points(context, swapchain.imageFormat());
        render::Renderer  renderer(context, swapchain);

        render::OrbitCamera camera;

        // Let the scroll callback reach the camera through the window.
        glfwSetWindowUserPointer(window, &camera);
        glfwSetScrollCallback(window, [](GLFWwindow* w, double, double yoff) {
            auto* cam = static_cast<render::OrbitCamera*>(glfwGetWindowUserPointer(w));
            cam->zoom(yoff > 0.0 ? 0.9f : 1.1f);   // scroll up = move closer
            });

        // Remember the cursor position so we can measure drag deltas.
        double lastX = 0.0, lastY = 0.0;
        glfwGetCursorPos(window, &lastX, &lastY);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Left-button drag orbits the camera.
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                float dx = static_cast<float>(mx - lastX);
                float dy = static_cast<float>(my - lastY);
                camera.orbit(-dx * 0.005f, -dy * 0.005f);   // 0.005 = sensitivity
            }
            lastX = mx;
            lastY = my;

            // Build this frame's camera matrices.
            VkExtent2D extent = swapchain.extent();
            float aspect = static_cast<float>(extent.width) /
                static_cast<float>(extent.height);
            Eigen::Matrix4f view = camera.viewMatrix();
            Eigen::Matrix4f proj = core::perspective(0.785398f, aspect, 0.1f, 100.0f);
            Eigen::Matrix4f viewProj = proj * view;

            renderer.drawFrame(points, viewProj);
        }
    }   // gpu::Context destroyed here

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}