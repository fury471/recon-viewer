#include "render/PointRenderable.h"

#include "gpu/Context.h"

#include <vk_mem_alloc.h>     // VMA declarations (no VMA_IMPLEMENTATION here)
#include <spdlog/spdlog.h>

#include <cstring>            // std::memcpy
#include <stdexcept>
#include <vector>

namespace render {

    PointRenderable::PointRenderable(const gpu::Context& ctx) : ctx_(ctx) {
        // A few test points: position (x,y,z) + color (r,g,b).
        std::vector<Vertex> vertices = {
            { {  0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },   // red
            { {  0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },   // green
            { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },   // blue
        };
        vertexCount_ = static_cast<uint32_t>(vertices.size());
        VkDeviceSize size = sizeof(Vertex) * vertices.size();

        // Describe the buffer: it holds vertex data.
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        // Ask VMA for memory the CPU can write to, kept permanently mapped.
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocResult{};
        if (vmaCreateBuffer(ctx.allocator(), &bufferInfo, &allocInfo,
            &buffer_, &allocation_, &allocResult) != VK_SUCCESS)
            throw std::runtime_error("render: vertex buffer creation failed");

        // Copy the points straight into the GPU buffer.
        std::memcpy(allocResult.pMappedData, vertices.data(), size);

        spdlog::info("render: point buffer ready ({} points)", vertexCount_);
    }

    PointRenderable::~PointRenderable() {
        if (buffer_)
            vmaDestroyBuffer(ctx_.allocator(), buffer_, allocation_);   // frees both
    }

}  // namespace render