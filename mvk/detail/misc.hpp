#ifndef MVK_DETAIL_MISC_HPP_INCLUDED
#define MVK_DETAIL_MISC_HPP_INCLUDED

#include "types/types.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"

#include <filesystem>
#include <optional>

namespace mvk::detail
{
  [[nodiscard]] utility::slice<std::byte> map_memory( types::device        device,
                                                      types::device_memory memory,
                                                      types::device_size   size   = VK_WHOLE_SIZE,
                                                      types::device_size   offset = 0 ) noexcept;

  [[nodiscard]] std::tuple<std::vector<unsigned char>, uint32_t, uint32_t>
    load_texture( std::filesystem::path const & path );

  [[nodiscard]] std::optional<uint32_t>
    find_memory_type( VkPhysicalDevice physical_device, uint32_t filter, VkMemoryPropertyFlags properties_flags );

  [[nodiscard]] std::optional<uint32_t>
    next_swapchain_image( VkDevice device, VkSwapchainKHR swapchain, VkSemaphore semaphore, VkFence fence );

  [[nodiscard]] uint32_t calculate_mimap_levels( uint32_t height, uint32_t width ) noexcept;

}  // namespace mvk::detail

#endif
