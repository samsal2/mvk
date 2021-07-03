#ifndef MVK_DETAIL_HELPERS_HPP_INCLUDED
#define MVK_DETAIL_HELPERS_HPP_INCLUDED

#include "utility/slice.hpp"

#include <optional>
#include <vulkan/vulkan.h>

namespace mvk::detail
{
  [[nodiscard]] bool is_extension_present( char const *                                  extension_name,
                                           utility::slice< VkExtensionProperties const > extensions ) noexcept;

  [[nodiscard]] bool check_extension_support( VkPhysicalDevice                     physical_device,
                                              utility::slice< char const * const > device_extensions ) noexcept;

  [[nodiscard]] bool check_graphic_requirements( VkQueueFamilyProperties const & queue_family ) noexcept;

  [[nodiscard]] bool check_format_and_present_mode_availability( VkPhysicalDevice physical_device,
                                                                 VkSurfaceKHR     surface ) noexcept;

  [[nodiscard]] bool
    check_surface_support( VkPhysicalDevice physical_device, VkSurfaceKHR surface, uint32_t index ) noexcept;

  [[nodiscard]] std::optional< std::pair< uint32_t, uint32_t > > query_family_indices( VkPhysicalDevice physical_device,
                                                                                       VkSurfaceKHR     surface );

  [[nodiscard]] uint32_t choose_image_count( VkSurfaceCapabilitiesKHR const & capabilities ) noexcept;

  [[nodiscard]] VkPresentModeKHR choose_present_mode( VkPhysicalDevice physical_device, VkSurfaceKHR surface ) noexcept;

  [[nodiscard]] VkExtent2D choose_extent( VkSurfaceCapabilitiesKHR const & capabilities,
                                          VkExtent2D const &               extent ) noexcept;

}  // namespace mvk::detail

#endif
