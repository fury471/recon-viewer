#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward-declare VMA's handle so this header need not include the heavy
// vk_mem_alloc.h. VK_DEFINE_HANDLE comes from <vulkan/vulkan.h>.
VK_DEFINE_HANDLE(VmaAllocator)

namespace gpu {

    struct ContextCreateInfo {
        std::string applicationName = "recon-viewer";
        std::vector<const char*> requiredInstanceExtensions;   // from GLFW
        std::function<VkSurfaceKHR(VkInstance)> createSurface;  // app supplies
        bool enableValidation = true;
    };

    // Owns the shared Vulkan context: instance, debug messenger, surface,
    // physical + logical device, queues, and the VMA allocator. Both the
    // renderer and the future GPU compute stage build on this one device.
    class Context {
    public:
        explicit Context(const ContextCreateInfo& info);
        ~Context();

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        VkInstance       instance() const;
        VkPhysicalDevice physicalDevice() const;
        VkDevice         device() const;
        VkSurfaceKHR     surface() const;

        VkQueue  graphicsQueue() const;
        VkQueue  presentQueue() const;
        uint32_t graphicsQueueFamily() const;
        uint32_t presentQueueFamily() const;

        VmaAllocator allocator() const;
        std::string  deviceName() const;

    private:
        struct Impl;                    // PIMPL: hides vk-bootstrap + VMA
        std::unique_ptr<Impl> impl_;
    };

}  // namespace gpu