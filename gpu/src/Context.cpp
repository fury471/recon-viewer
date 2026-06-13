#include "gpu/Context.h"

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION          // generate VMA's definitions in THIS file
#include <vk_mem_alloc.h>

#include <spdlog/spdlog.h>

#include <stdexcept>

namespace gpu {

    struct Context::Impl {
        vkb::Instance instance;
        vkb::Device   device;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkSurfaceKHR     surface = VK_NULL_HANDLE;
        VkQueue  graphicsQueue = VK_NULL_HANDLE;
        VkQueue  presentQueue = VK_NULL_HANDLE;
        uint32_t graphicsFamily = 0;
        uint32_t presentFamily = 0;
        VmaAllocator allocator = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties props{};
    };

    Context::Context(const ContextCreateInfo& info)
        : impl_(std::make_unique<Impl>()) {

        // 1. Instance — request Vulkan 1.3 (+ validation in debug) -----------
        vkb::InstanceBuilder builder;
        builder.set_app_name(info.applicationName.c_str())
            .require_api_version(1, 3, 0);
        if (info.enableValidation) {
            builder.request_validation_layers()
                .use_default_debug_messenger();
        }
        for (const char* ext : info.requiredInstanceExtensions)
            builder.enable_extension(ext);

        auto instRet = builder.build();
        if (!instRet)
            throw std::runtime_error("gpu: instance creation failed: "
                + instRet.error().message());
        impl_->instance = instRet.value();

        // 2. Surface (the app creates it from its window) --------------------
        impl_->surface = info.createSurface(impl_->instance.instance);
        if (impl_->surface == VK_NULL_HANDLE)
            throw std::runtime_error("gpu: window surface creation failed");

        // 3. Physical device: Vulkan 1.3 with dynamic rendering + sync2 ------
        //    These two are the modern baseline the renderer assumes. Dynamic
        //    rendering removes VkRenderPass/VkFramebuffer; synchronization2 is
        //    the current barrier/submit API. Both are core in Vulkan 1.3.
        //    We enable nothing we don't yet use.
        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;

        VkPhysicalDeviceFeatures features10{};
        features10.largePoints = VK_TRUE;   // allow point sizes larger than 1 pixel

        vkb::PhysicalDeviceSelector selector{ impl_->instance };
        auto physRet = selector.set_surface(impl_->surface)
            .set_minimum_version(1, 3)
            .set_required_features_13(features13)
            .set_required_features(features10)
            .select();
        if (!physRet)
            throw std::runtime_error("gpu: no suitable Vulkan 1.3 GPU: "
                + physRet.error().message());
        vkb::PhysicalDevice phys = physRet.value();
        impl_->physicalDevice = phys.physical_device;
        impl_->props = phys.properties;

        // 4. Logical device + queues (required features propagate here) ------
        auto devRet = vkb::DeviceBuilder{ phys }.build();
        if (!devRet)
            throw std::runtime_error("gpu: device creation failed: "
                + devRet.error().message());
        impl_->device = devRet.value();

        impl_->graphicsQueue = impl_->device.get_queue(vkb::QueueType::graphics).value();
        impl_->presentQueue = impl_->device.get_queue(vkb::QueueType::present).value();
        impl_->graphicsFamily = impl_->device.get_queue_index(vkb::QueueType::graphics).value();
        impl_->presentFamily = impl_->device.get_queue_index(vkb::QueueType::present).value();

        // 5. VMA allocator (target Vulkan 1.3) -------------------------------
        VmaAllocatorCreateInfo ai{};
        ai.physicalDevice = impl_->physicalDevice;
        ai.device = impl_->device.device;
        ai.instance = impl_->instance.instance;
        ai.vulkanApiVersion = VK_API_VERSION_1_3;
        if (vmaCreateAllocator(&ai, &impl_->allocator) != VK_SUCCESS)
            throw std::runtime_error("gpu: VMA allocator creation failed");

        spdlog::info("gpu: Vulkan 1.3 context ready on '{}'", impl_->props.deviceName);
    }

    Context::~Context() {
        if (!impl_) return;
        if (impl_->allocator) vmaDestroyAllocator(impl_->allocator);
        vkb::destroy_device(impl_->device);
        if (impl_->surface) vkb::destroy_surface(impl_->instance, impl_->surface);
        vkb::destroy_instance(impl_->instance);
        spdlog::info("gpu: Vulkan context destroyed");
    }

    VkInstance       Context::instance() const { return impl_->instance.instance; }
    VkPhysicalDevice Context::physicalDevice() const { return impl_->physicalDevice; }
    VkDevice         Context::device() const { return impl_->device.device; }
    VkSurfaceKHR     Context::surface() const { return impl_->surface; }
    VkQueue          Context::graphicsQueue() const { return impl_->graphicsQueue; }
    VkQueue          Context::presentQueue() const { return impl_->presentQueue; }
    uint32_t         Context::graphicsQueueFamily() const { return impl_->graphicsFamily; }
    uint32_t         Context::presentQueueFamily() const { return impl_->presentFamily; }
    VmaAllocator     Context::allocator() const { return impl_->allocator; }
    std::string      Context::deviceName() const { return impl_->props.deviceName; }

}  // namespace gpu