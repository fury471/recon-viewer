#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace gpu { class Context; }   // forward declaration — no heavy include here

namespace render {

    class Swapchain {
    public:
        Swapchain(const gpu::Context& ctx, uint32_t width, uint32_t height);
        ~Swapchain();

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        VkSwapchainKHR handle()      const { return swapchain_; }
        VkFormat       imageFormat() const { return imageFormat_; }
        VkExtent2D     extent()      const { return extent_; }
        uint32_t       imageCount()  const { return static_cast<uint32_t>(images_.size()); }
        const std::vector<VkImage>& images()     const { return images_; }
        const std::vector<VkImageView>& imageViews() const { return imageViews_; }

    private:
        const gpu::Context& ctx_;
        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
        VkFormat       imageFormat_ = VK_FORMAT_UNDEFINED;
        VkExtent2D     extent_{};
        std::vector<VkImage>     images_;
        std::vector<VkImageView> imageViews_;
    };

}  // namespace render