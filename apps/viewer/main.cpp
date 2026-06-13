#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "gpu/Context.h"
#include "render/Swapchain.h"
#include "render/Renderer.h"
#include "render/PointRenderable.h"

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

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            renderer.drawFrame(points);
        }
    }   // gpu::Context destroyed here

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}