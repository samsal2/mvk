#ifndef MVK_VK_TYPES_SWAPCHAIN_HPP_INCLUDED
#define MVK_VK_TYPES_SWAPCHAIN_HPP_INCLUDED

#include "utility/slice.hpp"
#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"
#include "vk_types/image_view.hpp"

#include <optional>
#include <vector>

namespace mvk::vk_types
{

class swapchain : public detail::wrapper<VkSwapchainKHR, vkDestroySwapchainKHR>
{
public:
        swapchain() noexcept = default;

        swapchain(VkDevice device, VkSwapchainCreateInfoKHR const & create_info);

        [[nodiscard]] constexpr VkExtent2D
        extent() const noexcept;

        [[nodiscard]] constexpr VkFormat
        format() const noexcept;

        [[nodiscard]] constexpr utility::slice<VkImage>
        images() const noexcept;

        [[nodiscard]] constexpr utility::slice<image_view>
        image_views() const noexcept;

        [[nodiscard]] std::optional<uint32_t>
        next_image(VkSemaphore semaphore, VkFence fence = nullptr) const noexcept;

private:
        std::vector<VkImage>    images_;
        std::vector<image_view> image_views_;
        VkExtent2D              extent_ = {};
        VkFormat                format_ = {};
};

[[nodiscard]] constexpr utility::slice<VkImage>
swapchain::images() const noexcept
{
        return {images_};
}

[[nodiscard]] constexpr utility::slice<image_view>
swapchain::image_views() const noexcept
{
        return {image_views_};
}

[[nodiscard]] constexpr VkExtent2D
swapchain::extent() const noexcept
{
        return extent_;
}

[[nodiscard]] constexpr VkFormat
swapchain::format() const noexcept
{
        return format_;
}

} // namespace mvk::vk_types

#endif // MVK_VK_TYPES_SWAPCHAIN_HPP_INCLUDED
