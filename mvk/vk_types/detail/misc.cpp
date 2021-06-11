#include "vk_types/detail/misc.hpp"

namespace mvk::vk_types::detail
{

[[nodiscard]] std::optional<uint32_t>
find_memory_type(VkPhysicalDevice const physical_device, uint32_t const filter, VkMemoryPropertyFlags const properties_flags)
{
    auto memory_properties = VkPhysicalDeviceMemoryProperties();
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    auto const type_count = memory_properties.memoryTypeCount;

    for (auto i = uint32_t(0); i < type_count; ++i)
    {
        auto const & current_type   = memory_properties.memoryTypes[i];
        auto const   current_flags  = current_type.propertyFlags;
        auto const   matches_flags  = (current_flags & properties_flags) != 0U;
        auto const   matches_filter = (filter & (1U << i)) != 0U;

        if (matches_flags && matches_filter)
        {
            return i;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<uint32_t>
next_swapchain_image(VkDevice const device, VkSwapchainKHR const swapchain, VkSemaphore const semaphore, VkFence const fence)
{

    auto index = uint32_t(0);

    auto const result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), semaphore, fence, &index);

    if (result != VK_ERROR_OUT_OF_DATE_KHR)
    {
        return index;
    }

    return std::nullopt;
}

[[nodiscard]] uint32_t
calculate_mimap_levels(uint32_t const height, uint32_t const width) noexcept
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(height, width))) + 1);
}

} // namespace mvk::vk_types::detail
