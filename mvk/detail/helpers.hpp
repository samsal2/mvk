#ifndef MVK_DETAIL_HELPERS_HPP_INCLUDED
#define MVK_DETAIL_HELPERS_HPP_INCLUDED

#include "types/types.hpp"
#include "utility/slice.hpp"

#include <optional>

namespace mvk::detail
{
[[nodiscard]] bool
is_extension_present(
    std::string const & extension_name,
    utility::slice<VkExtensionProperties const> extensions) noexcept;

[[nodiscard]] bool
check_extension_support(
    types::physical_device physical_device,
    utility::slice<char const * const> device_extensions) noexcept;

[[nodiscard]] std::optional<types::physical_device>
choose_physical_device(
    types::instance instance, types::surface surface,
    utility::slice<char const * const> device_extensions) noexcept;

[[nodiscard]] bool
check_graphic_requirements(
    VkQueueFamilyProperties const & queue_family) noexcept;

[[nodiscard]] bool
check_format_and_present_mode_availability(
    types::physical_device physical_device, types::surface surface) noexcept;

[[nodiscard]] bool
check_surface_support(types::physical_device physical_device,
                      types::surface surface, u32 index) noexcept;

[[nodiscard]] std::optional<std::pair<types::queue_index, types::queue_index>>
query_family_indices(types::physical_device physical_device,
                     types::surface surface);

[[nodiscard]] u32
choose_image_count(VkSurfaceCapabilitiesKHR const & capabilities) noexcept;

[[nodiscard]] VkPresentModeKHR
choose_present_mode(types::physical_device physical_device,
                    types::surface surface) noexcept;

[[nodiscard]] VkExtent2D
choose_extent(VkSurfaceCapabilitiesKHR const & capabilities,
              VkExtent2D const & extent) noexcept;

} // namespace mvk::detail

#endif
