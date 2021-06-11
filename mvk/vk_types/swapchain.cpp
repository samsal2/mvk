#include "vk_types/swapchain.hpp"

#include "vk_types/common.hpp"
#include "vk_types/detail/misc.hpp"
#include "vk_types/fence.hpp"
#include "vk_types/semaphore.hpp"

namespace mvk::vk_types
{

swapchain::swapchain(VkDevice const device, VkSwapchainCreateInfoKHR const & create_info)
    : unique_wrapper_with_parent(nullptr, device), extent_(create_info.imageExtent), format_(create_info.imageFormat)
{
    [[maybe_unused]] auto const result = vkCreateSwapchainKHR(parent(), &create_info, nullptr, &reference());

    MVK_VERIFY(VK_SUCCESS == result);

    auto images_size = uint32_t(0);
    vkGetSwapchainImagesKHR(parent(), get(), &images_size, nullptr);

    images_.resize(images_size);
    vkGetSwapchainImagesKHR(parent(), get(), &images_size, std::data(images_));

    image_views_.reserve(images_size);

    auto const add_image_view = [this](auto const image)
    {
        auto const image_view_create_info = [this, image]
        {
            auto info                            = VkImageViewCreateInfo();
            info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image                           = image;
            info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            info.format                          = format_;
            info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.baseMipLevel   = 0;
            info.subresourceRange.levelCount     = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount     = 1;
            return info;
        }();

        image_views_.emplace_back(parent(), image_view_create_info);
    };

    std::for_each(std::begin(images_), std::end(images_), add_image_view);
}

[[nodiscard]] std::optional<uint32_t>
swapchain::next_image(VkSemaphore const semaphore, VkFence const fence) const noexcept
{
    return detail::next_swapchain_image(parent(), get(), semaphore, fence);
}

} // namespace mvk::vk_types
