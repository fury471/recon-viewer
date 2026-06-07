#include "render/Renderer.h"

#include "render/Swapchain.h"
#include "gpu/Context.h"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <string>

namespace render {
    static std::vector<char> readFile(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);  // open at end
        if (!file) throw std::runtime_error("render: cannot open shader: " + path);
        size_t size = static_cast<size_t>(file.tellg());             // position = size
        std::vector<char> buffer(size);
        file.seekg(0);
        file.read(buffer.data(), size);
        return buffer;
    }

    static VkShaderModule loadShaderModule(VkDevice device, const std::string& path) {
        std::vector<char> code = readFile(path);
        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.size();
        info.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule module = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device, &info, nullptr, &module) != VK_SUCCESS)
            throw std::runtime_error("render: shader module creation failed: " + path);
        return module;
    }

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

        createTrianglePipeline();
    }

    void Renderer::drawFrame() {
        VkDevice device = ctx_.device();

        // [top of loop] Wait for the previous frame's GPU work to finish, then
        // un-signal the fence so it can mark THIS frame done. (GPU -> CPU brake.)
        vkWaitForFences(device, 1, &inFlightFence_, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence_);

        // [box 1: ACQUIRE] Borrow a free plate. imageAvailable_ is raised when
        // the image is actually ready; we get back which plate (imageIndex).
        uint32_t imageIndex = 0;
        vkAcquireNextImageKHR(device, swapchain_.handle(), UINT64_MAX,
            imageAvailable_, VK_NULL_HANDLE, &imageIndex);

        // [box 2: RECORD] Write a fresh ticket.
        vkResetCommandBuffer(commandBuffer_, 0);

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer_, &begin);

        VkImage     image = swapchain_.images()[imageIndex];
        VkImageView view = swapchain_.imageViews()[imageIndex];

        // mode: nothing -> drawable
        transitionImage(commandBuffer_, image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // dynamic rendering: attach the plate, and CLEAR it to our color.
        // (Clearing IS our first light — there's nothing else to draw yet.)
        VkClearValue clear{};
        clear.color = { { 0.05f, 0.10f, 0.18f, 1.0f } };   // calm dark blue

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
        vkCmdEndRendering(commandBuffer_);

        // mode: drawable -> presentable
        transitionImage(commandBuffer_, image,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vkEndCommandBuffer(commandBuffer_);

        // [box 3: SUBMIT] Clip the ticket to the graphics queue. Wait on
        // imageAvailable_ before drawing; raise this plate's bell when done;
        // and signal inFlightFence_ so next frame's wait knows we're finished.
        VkCommandBufferSubmitInfo cmdInfo{};
        cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmdInfo.commandBuffer = commandBuffer_;

        VkSemaphoreSubmitInfo wait{};
        wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait.semaphore = imageAvailable_;
        wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signal{};
        signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal.semaphore = renderFinished_[imageIndex];   // <-- the per-plate bell
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

        // [box 4: PRESENT] Serve the plate. Wait on this plate's bell first.
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

    void Renderer::createTrianglePipeline() {
        VkDevice device = ctx_.device();

        VkShaderModule vert = loadShaderModule(device, SHADER_OUTPUT_DIR "/triangle.vert.spv");
        VkShaderModule frag = loadShaderModule(device, SHADER_OUTPUT_DIR "/triangle.frag.spv");

        // [MATTERS] The two programmable stages: our vertex + fragment shaders.
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vert;
        stages[0].pName = "main";                  // the entry-point function name
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = frag;
        stages[1].pName = "main";

        // [recipe] No vertex buffers — corners are hardcoded in the shader.
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        // [MATTERS] We're assembling triangles.
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // [recipe] Viewport + scissor are set at draw time (dynamic); just say "one each".
        VkPipelineViewportStateCreateInfo viewport{};
        viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;

        // [recipe] Fill the triangle, no back-face culling.
        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
        raster.lineWidth = 1.0f;

        // [recipe] No multisampling.
        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // [recipe] One color output, no blending (just write the pixel).
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAttachment;

        // [recipe] Which bits we'll set per-frame instead of baking in.
        VkDynamicState dynamics[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamics;

        // [recipe] Empty layout: no descriptor sets or push constants yet.
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &trianglePipelineLayout_)
            != VK_SUCCESS)
            throw std::runtime_error("render: pipeline layout creation failed");

        // [MATTERS] Dynamic rendering: instead of a render pass, tell the pipeline
        // the color format it'll draw into (the swapchain's format).
        VkFormat colorFormat = swapchain_.imageFormat();
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &colorFormat;

        // Assemble the whole form.
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &renderingInfo;   // <- dynamic rendering
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewport;
        pipelineInfo.pRasterizationState = &raster;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = trianglePipelineLayout_;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
            nullptr, &trianglePipeline_) != VK_SUCCESS)
            throw std::runtime_error("render: graphics pipeline creation failed");

        // Shader modules are only needed to build the pipeline — free them now.
        vkDestroyShaderModule(device, frag, nullptr);
        vkDestroyShaderModule(device, vert, nullptr);

        spdlog::info("render: triangle pipeline ready");
    }

    Renderer::~Renderer() {
        vkDeviceWaitIdle(ctx_.device());   // let ALL in-flight GPU work finish first

        if (trianglePipeline_)
            vkDestroyPipeline(ctx_.device(), trianglePipeline_, nullptr);
        if (trianglePipelineLayout_)
            vkDestroyPipelineLayout(ctx_.device(), trianglePipelineLayout_, nullptr);
        for (VkSemaphore sem : renderFinished_)
            vkDestroySemaphore(ctx_.device(), sem, nullptr);
        if (inFlightFence_)  vkDestroyFence(ctx_.device(), inFlightFence_, nullptr);
        if (imageAvailable_) vkDestroySemaphore(ctx_.device(), imageAvailable_, nullptr);
        if (commandPool_)    vkDestroyCommandPool(ctx_.device(), commandPool_, nullptr);
    }

}  // namespace render