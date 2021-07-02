#ifndef MVK_DETAIL_MISC_HPP_INCLUDED
#define MVK_DETAIL_MISC_HPP_INCLUDED

#include "detail/checkers.hpp"
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

  [[nodiscard]] std::tuple<std::vector<unsigned char>, u32, u32> load_texture( std::filesystem::path const & path );

  template <typename Checker>
  requires requirement_checker<Checker>
  [[nodiscard]] static VkSurfaceFormatKHR
    choose_surface_format( VkPhysicalDevice physical_device, VkSurfaceKHR surface, Checker && check ) noexcept;

  [[nodiscard]] std::optional<u32>
    find_memory_type( VkPhysicalDevice physical_device, u32 filter, VkMemoryPropertyFlags properties_flags );

  [[nodiscard]] std::optional<u32>
    next_swapchain_image( VkDevice device, VkSwapchainKHR swapchain, VkSemaphore semaphore, VkFence fence );

  [[nodiscard]] u32 calculate_mimap_levels( u32 height, u32 width ) noexcept;

  template <typename Checker>
  requires requirement_checker<Checker>
  [[nodiscard]] static VkSurfaceFormatKHR
    choose_surface_format( VkPhysicalDevice physical_device, VkSurfaceKHR surface, Checker && check ) noexcept
  {
    auto formats_count = u32( 0 );
    vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &formats_count, nullptr );

    auto formats = std::vector<VkSurfaceFormatKHR>( formats_count );
    vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &formats_count, std::data( formats ) );

    auto const it = std::find_if( std::begin( formats ), std::end( formats ), std::forward<Checker>( check ) );

    if ( it != std::end( formats ) )
    {
      return *it;
    }

    return formats[0];
  }
}  // namespace mvk::detail

#endif
