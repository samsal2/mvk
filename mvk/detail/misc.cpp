#include "detail/misc.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma clang diagnostic pop

#include <array>
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace mvk::detail
{
  [[nodiscard]] std::tuple<std::vector<unsigned char>, uint32_t, uint32_t>
    load_texture( std::filesystem::path const & path )
  {
    MVK_VERIFY( std::filesystem::exists( path ) );

    auto       width    = 0;
    auto       height   = 0;
    auto       channels = 0;
    auto const pixels   = stbi_load( path.c_str(), &width, &height, &channels, STBI_rgb_alpha );

    auto buffer = std::vector<unsigned char>( static_cast<uint32_t>( width ) * static_cast<uint32_t>( height ) * 4 *
                                              sizeof( *pixels ) );
    std::copy( pixels, std::next( pixels, static_cast<int64_t>( std::size( buffer ) ) ), std::begin( buffer ) );

    stbi_image_free( pixels );

    return { std::move( buffer ), width, height };
  }

  [[nodiscard]] std::optional<uint32_t> find_memory_type( VkPhysicalDevice const      physical_device,
                                                          uint32_t const              filter,
                                                          VkMemoryPropertyFlags const properties_flags )
  {
    auto memory_properties = VkPhysicalDeviceMemoryProperties();
    vkGetPhysicalDeviceMemoryProperties( physical_device, &memory_properties );

    auto const type_count = memory_properties.memoryTypeCount;

    for ( auto i = uint32_t( 0 ); i < type_count; ++i )
    {
      auto const & current_type   = memory_properties.memoryTypes[i];
      auto const   current_flags  = current_type.propertyFlags;
      auto const   matches_flags  = ( current_flags & properties_flags ) != 0U;
      auto const   matches_filter = ( filter & ( 1U << i ) ) != 0U;

      if ( matches_flags && matches_filter )
      {
        return i;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint32_t> next_swapchain_image( VkDevice const       device,
                                                              VkSwapchainKHR const swapchain,
                                                              VkSemaphore const    semaphore,
                                                              VkFence const        fence )
  {
    auto index = uint32_t( 0 );

    auto const result =
      vkAcquireNextImageKHR( device, swapchain, std::numeric_limits<uint64_t>::max(), semaphore, fence, &index );

    if ( result != VK_ERROR_OUT_OF_DATE_KHR )
    {
      return index;
    }

    return std::nullopt;
  }

  [[nodiscard]] uint32_t calculate_mimap_levels( uint32_t const height, uint32_t const width ) noexcept
  {
    return static_cast<uint32_t>( std::floor( std::log2( std::max( height, width ) ) ) + 1 );
  }

  [[nodiscard]] utility::slice<std::byte> map_memory( types::device const        device,
                                                      types::device_memory const memory,
                                                      types::device_size const   size,
                                                      types::device_size const   offset ) noexcept
  {
    void * data = nullptr;
    vkMapMemory( types::get( device ), types::get( memory ), offset, size, 0, &data );
    return { utility::force_cast_to_byte( data ), size };
  }
}  // namespace mvk::detail
