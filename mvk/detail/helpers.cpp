#include "detail/helpers.hpp"

#include "detail/misc.hpp"
#include "detail/query.hpp"

#include <algorithm>

namespace mvk::detail
{

[[nodiscard]] bool
is_extension_present(
    std::string const & extension_name,
    utility::slice<VkExtensionProperties> const extensions) noexcept
{
  auto const matches = [&extension_name](auto const & extension)
  {
    auto const name = static_cast<char const *>(extension.extensionName);
    return std::strcmp(extension_name.c_str(), name) == 0;
  };
  return std::any_of(std::begin(extensions), std::end(extensions), matches);
}

[[nodiscard]] bool
check_extension_support(
    types::physical_device physical_device,
    utility::slice<char const *> device_extensions) noexcept
{
  auto const extensions = query<vkEnumerateDeviceExtensionProperties>::with(
      types::get(physical_device), nullptr);

  auto is_present = [&extensions](auto const & extension)
  {
    return is_extension_present(extension, extensions);
  };

  auto const begin = std::begin(device_extensions);
  auto const end = std::end(device_extensions);
  return std::all_of(begin, end, is_present);
}

[[nodiscard]] std::optional<types::physical_device>
choose_physical_device(
    types::instance const & instance, types::surface const & surface,
    utility::slice<char const *> const device_extensions) noexcept
{
  auto const physical_devices =
      query<vkEnumeratePhysicalDevices>::with(types::get(instance));

  auto const supported =
      [device_extensions, &surface](auto const physical_device)
  {
    auto const features =
        query<vkGetPhysicalDeviceFeatures>::with(physical_device);

    auto const extensions_supported =
        check_extension_support(physical_device, device_extensions);

    auto const format_and_present_mode_available =
        check_format_and_present_mode_availability(physical_device, surface);

    auto const family_indices_found =
        query_family_indices(physical_device, surface);

    return extensions_supported && format_and_present_mode_available &&
           family_indices_found && features.samplerAnisotropy;
  };

  auto const begin = std::begin(physical_devices);
  auto const end = std::end(physical_devices);
  auto const it = std::find_if(begin, end, supported);

  if (it != std::end(physical_devices))
  {
    return *it;
  }

  return std::nullopt;
}

[[nodiscard]] bool
check_graphic_requirements(
    VkQueueFamilyProperties const & queue_family) noexcept
{
  return (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U;
}

[[nodiscard]] bool
check_format_and_present_mode_availability(
    types::physical_device physical_device,
    types::surface const & surface) noexcept
{
  auto format_count = uint32_t(0);
  vkGetPhysicalDeviceSurfaceFormatsKHR(types::get(physical_device),
                                       types::get(surface), &format_count,
                                       nullptr);

  auto present_mode_count = uint32_t(0);
  vkGetPhysicalDeviceSurfacePresentModesKHR(types::get(physical_device),
                                            types::get(surface),
                                            &present_mode_count, nullptr);

  return format_count != 0 && present_mode_count != 0;
}

[[nodiscard]] bool
check_surface_support(types::physical_device const physical_device,
                      types::surface const & surface, uint32_t index) noexcept
{

  auto supported = VkBool32(false);
  vkGetPhysicalDeviceSurfaceSupportKHR(types::get(physical_device), index,
                                       types::get(surface), &supported);

  return supported != 0U;
}

[[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>>
query_family_indices(types::physical_device const physical_device,
                     types::surface const & surface)
{
  auto const queue_families =
      query<vkGetPhysicalDeviceQueueFamilyProperties>::with(
          types::get(physical_device));

  auto graphics_family = std::optional<uint32_t>();
  auto present_family = std::optional<uint32_t>();

  auto const is_complete = [&, i = 0U](auto const & queue_family) mutable
  {
    if (queue_family.queueCount == 0)
    {
      ++i;
      return false;
    }

    if (check_graphic_requirements(queue_family))
    {
      graphics_family = i;
    }

    if (check_surface_support(physical_device, surface, i))
    {
      present_family = i;
    }

    auto const families_have_value =
        graphics_family.has_value() && present_family.has_value();

    i += !families_have_value;

    return families_have_value;
  };

  auto const family = std::find_if(std::begin(queue_families),
                                   std::end(queue_families), is_complete);

  if (family != std::end(queue_families))
  {
    return std::make_optional(
        std::make_pair(graphics_family.value(), present_family.value()));
  }

  return std::nullopt;
}

[[nodiscard]] uint32_t
choose_image_count(VkSurfaceCapabilitiesKHR const & capabilities) noexcept
{
  auto const candidate = capabilities.minImageCount + 1;
  auto const max_limit = capabilities.maxImageCount;

  if (max_limit > 0 && candidate > max_limit)
  {
    return max_limit;
  }

  return candidate;
}

[[nodiscard]] VkPresentModeKHR
choose_present_mode(types::physical_device physical_device,
                    types::surface const & surface) noexcept
{
  auto const modes = query<vkGetPhysicalDeviceSurfacePresentModesKHR>::with(
      types::get(physical_device), types::get(surface));

  auto const it = std::find(std::begin(modes), std::end(modes),
                            VK_PRESENT_MODE_MAILBOX_KHR);

  if (it != std::end(modes))
  {
    return *it;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard]] VkExtent2D
choose_extent(VkSurfaceCapabilitiesKHR const & capabilities,
              VkExtent2D const & extent) noexcept
{
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }

  auto const min_width = capabilities.minImageExtent.width;
  auto const min_height = capabilities.minImageExtent.height;
  auto const max_width = capabilities.maxImageExtent.width;
  auto const max_height = capabilities.maxImageExtent.height;

  auto const extent_width = std::clamp(extent.width, min_width, max_width);
  auto const extent_height =
      std::clamp(extent.height, min_height, max_height);

  return {extent_width, extent_height};
}

} // namespace mvk::detail
