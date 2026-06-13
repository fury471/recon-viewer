#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace gpu { class Context; }

namespace render {

    class Swapchain;
    class PointRenderable;

    class Renderer {
    public:
        explicit Renderer(const gpu::Context& ctx, const Swapchain& swapchain);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        void drawFrame(const PointRenderable& points);

    private:
        const gpu::Context& ctx_;
        const Swapchain& swapchain_;

        VkCommandPool   commandPool_ = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;

        VkSemaphore              imageAvailable_ = VK_NULL_HANDLE;  // per frame
        VkFence                  inFlightFence_ = VK_NULL_HANDLE;   // per frame
        std::vector<VkSemaphore> renderFinished_;                   // one per image

    };

}  // namespace render