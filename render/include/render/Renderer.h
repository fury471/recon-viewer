#pragma once

#include <vulkan/vulkan.h>

namespace gpu { class Context; }

namespace render {

    class Renderer {
    public:
        explicit Renderer(const gpu::Context& ctx);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

    private:
        const gpu::Context& ctx_;
        VkCommandPool   commandPool_ = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    };

}  // namespace render