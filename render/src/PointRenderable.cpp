#include "render/PointRenderable.h"

#include "gpu/Context.h"

#include <vk_mem_alloc.h>
#include <spdlog/spdlog.h>

#include <cstddef>     // offsetof
#include <cstring>     // memcpy
#include <fstream>
#include <stdexcept>
#include <vector>

namespace render {

    static std::vector<char> readFile(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file) throw std::runtime_error("render: cannot open shader: " + path);
        size_t size = static_cast<size_t>(file.tellg());
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

    PointRenderable::PointRenderable(const gpu::Context& ctx, VkFormat colorFormat)
        : ctx_(ctx) {
        // ----- vertex buffer (unchanged from step 1) -----
        std::vector<Vertex> vertices = {
            { {  0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            { {  0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        };
        vertexCount_ = static_cast<uint32_t>(vertices.size());
        VkDeviceSize size = sizeof(Vertex) * vertices.size();

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocResult{};
        if (vmaCreateBuffer(ctx.allocator(), &bufferInfo, &allocInfo,
            &buffer_, &allocation_, &allocResult) != VK_SUCCESS)
            throw std::runtime_error("render: vertex buffer creation failed");
        std::memcpy(allocResult.pMappedData, vertices.data(), size);

        // ----- pipeline (new) -----
        createPipeline(colorFormat);

        spdlog::info("render: point renderable ready ({} points)", vertexCount_);
    }

    void PointRenderable::createPipeline(VkFormat colorFormat) {
        VkDevice device = ctx_.device();

        VkShaderModule vert = loadShaderModule(device, SHADER_OUTPUT_DIR "/point.vert.spv");
        VkShaderModule frag = loadShaderModule(device, SHADER_OUTPUT_DIR "/point.frag.spv");

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vert;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = frag;
        stages[1].pName = "main";

        // [DIFFERENT #1] Describe how to read one Vertex from the buffer:
        // 24-byte stride, two vec3 attributes (position at 0, color at 12).
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attrs[2]{};
        attrs[0].location = 0;                            // matches point.vert location 0
        attrs[0].binding = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;   // a vec3
        attrs[0].offset = offsetof(Vertex, position);
        attrs[1].location = 1;                            // matches point.vert location 1
        attrs[1].binding = 0;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, color);

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = 2;
        vertexInput.pVertexAttributeDescriptions = attrs;

        // [DIFFERENT #2] Points, not triangles.
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        VkPipelineViewportStateCreateInfo viewport{};
        viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
        raster.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAttachment;

        VkDynamicState dynamics[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamics;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;   // smaller depth = nearer wins

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(float) * 16;   // one mat4 = 64 bytes

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
            throw std::runtime_error("render: point pipeline layout creation failed");

        // [DIFFERENT #3] The color format is passed in (the swapchain's format).
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &colorFormat;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewport;
        pipelineInfo.pRasterizationState = &raster;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout_;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
            nullptr, &pipeline_) != VK_SUCCESS)
            throw std::runtime_error("render: point pipeline creation failed");

        vkDestroyShaderModule(device, frag, nullptr);
        vkDestroyShaderModule(device, vert, nullptr);
    }

    PointRenderable::~PointRenderable() {
        if (pipeline_)       vkDestroyPipeline(ctx_.device(), pipeline_, nullptr);
        if (pipelineLayout_) vkDestroyPipelineLayout(ctx_.device(), pipelineLayout_, nullptr);
        if (buffer_)         vmaDestroyBuffer(ctx_.allocator(), buffer_, allocation_);
    }

}  // namespace render