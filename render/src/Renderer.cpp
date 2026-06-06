#include "render/Renderer.h"

#include "gpu/Context.h"

#include <spdlog/spdlog.h>

#include <stdexcept>

namespace render {

    Renderer::Renderer(const gpu::Context& ctx) : ctx_(ctx) {
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

        spdlog::info("render: command pool and buffer ready");
    }

    Renderer::~Renderer() {
        // Destroying the pool frees every command buffer it handed out, so there's
        // nothing else to clean up here.
        if (commandPool_)
            vkDestroyCommandPool(ctx_.device(), commandPool_, nullptr);
    }

}  // namespace render