#include "render/Renderer.h"
#include "render/PointRenderable.h"

#include "render/Swapchain.h"
#include "gpu/Context.h"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vector>
#include <string>

namespace render {
    // Changes a swapchain image's "mode". The exact stage/access masks below are
    // the broad, always-correct recipe (we can tighten them for speed later).
    static void transitionImage(VkCommandBuffer cmd, VkImage image,
        VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.image = image;
        barrier.subresourceRange =
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        VkDependencyInfo dep{};
        dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(cmd, &dep);   // a Vulkan 1.3 / synchronization2 call
    }

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

    void Renderer::drawFrame(const PointRenderable& points) {
        VkDevice device = ctx_.device();

        vkWaitForFences(device, 1, &inFlightFence_, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence_);

        uint32_t imageIndex = 0;
        vkAcquireNextImageKHR(device, swapchain_.handle(), UINT64_MAX,
            imageAvailable_, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(commandBuffer_, 0);

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer_, &begin);

        VkImage     image = swapchain_.images()[imageIndex];
        VkImageView view = swapchain_.imageViews()[imageIndex];

        transitionImage(commandBuffer_, image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkClearValue clear{};
        clear.color = { { 0.05f, 0.10f, 0.18f, 1.0f } };

        VkRenderingAttachmentInfo color{};
        color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color.imageView = view;
        color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.clearValue = clear;

        VkRenderingInfo render{};
        render.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        render.renderArea.extent = swapchain_.extent();
        render.layerCount = 1;
        render.colorAttachmentCount = 1;
        render.pColorAttachments = &color;

        vkCmdBeginRendering(commandBuffer_, &render);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchain_.extent().width);
        viewport.height = static_cast<float>(swapchain_.extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapchain_.extent();
        vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);

        // ===== draw the point cloud =====
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, points.pipeline());

        VkBuffer     vertexBuffers[] = { points.buffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer_, 0, 1, vertexBuffers, offsets);

        vkCmdDraw(commandBuffer_, points.vertexCount(), 1, 0, 0);
        // ===== end draw =====

        vkCmdEndRendering(commandBuffer_);

        transitionImage(commandBuffer_, image,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vkEndCommandBuffer(commandBuffer_);

        VkCommandBufferSubmitInfo cmdInfo{};
        cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmdInfo.commandBuffer = commandBuffer_;

        VkSemaphoreSubmitInfo wait{};
        wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait.semaphore = imageAvailable_;
        wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signal{};
        signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal.semaphore = renderFinished_[imageIndex];
        signal.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

        VkSubmitInfo2 submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit.waitSemaphoreInfoCount = 1;
        submit.pWaitSemaphoreInfos = &wait;
        submit.commandBufferInfoCount = 1;
        submit.pCommandBufferInfos = &cmdInfo;
        submit.signalSemaphoreInfoCount = 1;
        submit.pSignalSemaphoreInfos = &signal;

        vkQueueSubmit2(ctx_.graphicsQueue(), 1, &submit, inFlightFence_);

        VkSwapchainKHR sc = swapchain_.handle();
        VkPresentInfoKHR present{};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &renderFinished_[imageIndex];
        present.swapchainCount = 1;
        present.pSwapchains = &sc;
        present.pImageIndices = &imageIndex;

        vkQueuePresentKHR(ctx_.presentQueue(), &present);
    }

    Renderer::~Renderer() {
        vkDeviceWaitIdle(ctx_.device());   // let ALL in-flight GPU work finish first
        for (VkSemaphore sem : renderFinished_)
            vkDestroySemaphore(ctx_.device(), sem, nullptr);
        if (inFlightFence_)  vkDestroyFence(ctx_.device(), inFlightFence_, nullptr);
        if (imageAvailable_) vkDestroySemaphore(ctx_.device(), imageAvailable_, nullptr);
        if (commandPool_)    vkDestroyCommandPool(ctx_.device(), commandPool_, nullptr);
    }

}  // namespace render