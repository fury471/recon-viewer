#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

VK_DEFINE_HANDLE(VmaAllocation)   // forward-declare VMA's handle (like in Context.h)

namespace gpu { class Context; }

namespace render {

    // One point: where it is, and what color it is. 3 floats each = 24 bytes.
    struct Vertex {
        float position[3];
        float color[3];
    };

    // A cloud of points living in GPU memory, ready to be drawn later.
    class PointRenderable {
    public:
        explicit PointRenderable(const gpu::Context& ctx);
        ~PointRenderable();

        PointRenderable(const PointRenderable&) = delete;
        PointRenderable& operator=(const PointRenderable&) = delete;

        VkBuffer buffer()      const { return buffer_; }
        uint32_t vertexCount() const { return vertexCount_; }

    private:
        const gpu::Context& ctx_;
        VkBuffer      buffer_ = VK_NULL_HANDLE;
        VmaAllocation allocation_ = VK_NULL_HANDLE;
        uint32_t      vertexCount_ = 0;
    };

}  // namespace render