#ifndef MVK_VK_TYPES_DETAIL_MISC_HPP_INCLUDED
#define MVK_VK_TYPES_DETAIL_MISC_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/checkers.hpp"

#include <optional>
#include <vector>

namespace mvk::vk_types::detail
{

template <typename Checker>
requires requirement_checker<Checker>
[[nodiscard]] static VkSurfaceFormatKHR
choose_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface, Checker && check) noexcept;

[[nodiscard]] std::optional<uint32_t>
find_memory_type(VkPhysicalDevice physical_device, uint32_t filter, VkMemoryPropertyFlags properties_flags);

[[nodiscard]] std::optional<uint32_t>
next_swapchain_image(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore semaphore, VkFence fence);

[[nodiscard]] uint32_t
calculate_mimap_levels(uint32_t height, uint32_t width) noexcept;

} // namespace mvk::vk_types::detail

namespace mvk::vk_types::detail
{

template <typename Checker>
requires requirement_checker<Checker>
[[nodiscard]] static VkSurfaceFormatKHR
choose_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface, Checker && check) noexcept
{
        auto count = uint32_t(0);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr);

        auto formats = std::vector<VkSurfaceFormatKHR>(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, std::data(formats));

        auto const it = std::find_if(std::begin(formats), std::end(formats), std::forward<Checker>(check));

        if (it != std::end(formats))
        {
                return *it;
        }

        return formats[0];
}

} // namespace mvk::vk_types::detail

#endif
