#include "render/Swapchain.h"

#include "gpu/Context.h"

#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

#include <stdexcept>

namespace render {

    Swapchain::Swapchain(const gpu::Context& ctx, uint32_t width, uint32_t height)
        : ctx_(ctx) {
        vkb::SwapchainBuilder builder{ ctx.physicalDevice(), ctx.device(), ctx.surface() };

        auto ret = builder
            .set_desired_format(VkSurfaceFormatKHR{
                VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .build();
        if (!ret)
            throw std::runtime_error("render: swapchain creation failed: "
                + ret.error().message());

        vkb::Swapchain vkbSwap = ret.value();
        swapchain_ = vkbSwap.swapchain;
        imageFormat_ = vkbSwap.image_format;
        extent_ = vkbSwap.extent;
        images_ = vkbSwap.get_images().value();
        imageViews_ = vkbSwap.get_image_views().value();

        spdlog::info("render: swapchain created - {} images at {}x{}",
            images_.size(), extent_.width, extent_.height);
    }

    Swapchain::~Swapchain() {
        for (VkImageView view : imageViews_)
            vkDestroyImageView(ctx_.device(), view, nullptr);
        if (swapchain_)
            vkDestroySwapchainKHR(ctx_.device(), swapchain_, nullptr);
    }

}  // namespace render