#include "render/Renderer.h"

#include "render/Swapchain.h"
#include "gpu/Context.h"

#include <spdlog/spdlog.h>

#include <stdexcept>

namespace render {

    Renderer::Renderer(const gpu::Context& ctx, const Swapchain& swapchain) : ctx_(ctx), swapchain_(swapchain) {
        // Command pool — the allocator that command buffers come from. It's tied
        // to the graphics queue family (the lane its tickets will run on), and the
        // RESET flag lets us wipe and re-record the buffer fresh every frame.
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = ctx.graphicsQueueFamily();
        if (vkCreateCommandPool(ctx.device(), &poolInfo, nullptr, &commandPool_)
            != VK_SUCCESS)
            throw std::runtime_error("render: command pool creation failed");

        // One command buffer allocated from that pool — our blank ticket.
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool_;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(ctx.device(), &allocInfo, &commandBuffer_)
            != VK_SUCCESS)
            throw std::runtime_error("render: command buffer allocation failed");

        // --------------- Sync objects  --------------------
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;   // start "already done"

        if (vkCreateSemaphore(ctx.device(), &semInfo, nullptr, &imageAvailable_)
            != VK_SUCCESS ||
            vkCreateFence(ctx.device(), &fenceInfo, nullptr, &inFlightFence_)
            != VK_SUCCESS)
            throw std::runtime_error("render: per-frame sync creation failed");

        renderFinished_.resize(swapchain.imageCount());
        for (VkSemaphore& sem : renderFinished_)
            if (vkCreateSemaphore(ctx.device(), &semInfo, nullptr, &sem)
                != VK_SUCCESS)
                throw std::runtime_error("render: render-finished sem creation failed");

        spdlog::info("render: sync objects ready ({} render-finished semaphores)",
            renderFinished_.size());
    }

    Renderer::~Renderer() {
        for (VkSemaphore sem : renderFinished_)
            vkDestroySemaphore(ctx_.device(), sem, nullptr);
        if (inFlightFence_)  vkDestroyFence(ctx_.device(), inFlightFence_, nullptr);
        if (imageAvailable_) vkDestroySemaphore(ctx_.device(), imageAvailable_, nullptr);
        if (commandPool_)    vkDestroyCommandPool(ctx_.device(), commandPool_, nullptr);
    }

}  // namespace render