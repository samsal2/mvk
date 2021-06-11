#include "detail/misc.hpp"

#include "utility/misc.hpp"

#include <array>
#include <optional>
#include <utility>

namespace mvk::detail
{

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
choose_present_mode(VkPhysicalDevice const physical_device, VkSurfaceKHR const surface) noexcept
{
    auto count = uint32_t(0);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr);

    auto modes = std::vector<VkPresentModeKHR>(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, std::data(modes));

    auto const it = std::find(std::begin(modes), std::end(modes), VK_PRESENT_MODE_MAILBOX_KHR);

    if (it != std::end(modes))
    {
        return *it;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard]] VkExtent2D
choose_extent(VkSurfaceCapabilitiesKHR const & capabilities, VkExtent2D const & extent) noexcept
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    auto const min_width  = capabilities.minImageExtent.width;
    auto const min_height = capabilities.minImageExtent.height;
    auto const max_width  = capabilities.maxImageExtent.width;
    auto const max_height = capabilities.maxImageExtent.height;

    auto const extent_width  = std::clamp(extent.width, min_width, max_width);
    auto const extent_height = std::clamp(extent.height, min_height, max_height);

    return {extent_width, extent_height};
}

[[nodiscard]] bool
is_extension_present(std::string const & current_extension_name, utility::slice<VkExtensionProperties> const extensions) noexcept
{
    auto const matches = [&current_extension_name](auto const & extension)
    {
        auto const ext_name = static_cast<char const *>(extension.extensionName);
        return std::strcmp(current_extension_name.c_str(), ext_name) == 0;
    };
    return std::any_of(std::begin(extensions), std::end(extensions), matches);
}

[[nodiscard]] bool
check_extension_support(VkPhysicalDevice const physical_device, utility::slice<char const *> const device_extensions) noexcept
{
    auto count = uint32_t(0);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, nullptr);

    auto extensions = std::vector<VkExtensionProperties>(count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, std::data(extensions));

    auto is_present = [&extensions](auto const & extension)
    {
        return is_extension_present(extension, extensions);
    };

    auto const begin_extensions = std::begin(device_extensions);
    auto const end_extensions   = std::end(device_extensions);
    return std::all_of(begin_extensions, end_extensions, is_present);
}

[[nodiscard]] bool
check_format_and_present_mode_availability(VkPhysicalDevice const physical_device, VkSurfaceKHR const surface) noexcept
{
    auto format_count = uint32_t(0);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

    auto present_mode_count = uint32_t(0);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

    return format_count != 0 && present_mode_count != 0;
}

[[nodiscard]] constexpr bool
meets_graphic_requirements(VkQueueFamilyProperties const & queue_family) noexcept
{
    return (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U;
}

[[nodiscard]] bool
supports_surface(VkPhysicalDevice const physical_device, VkSurfaceKHR const surface, uint32_t const index)
{
    auto supported = VkBool32(false);
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface, &supported);
    return supported != 0U;
}

[[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>>
query_family_indices(VkPhysicalDevice const physical_device, VkSurfaceKHR const surface)
{
    auto count = uint32_t(0);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);

    auto queue_families = std::vector<VkQueueFamilyProperties>(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, std::data(queue_families));

    auto graphics_family = std::optional<uint32_t>();
    auto present_family  = std::optional<uint32_t>();

    auto const is_complete = [&, i = 0U](auto const & queue_family) mutable
    {
        if (queue_family.queueCount == 0)
        {
            ++i;
            return false;
        }

        if (meets_graphic_requirements(queue_family))
        {
            graphics_family = i;
        }

        if (supports_surface(physical_device, surface, i))
        {
            present_family = i;
        }

        auto const families_have_value = graphics_family.has_value() && present_family.has_value();

        i += !families_have_value;

        return families_have_value;
    };

    auto const family = std::find_if(std::begin(queue_families), std::end(queue_families), is_complete);

    if (family != std::end(queue_families))
    {
        return std::make_optional(std::make_pair(graphics_family.value(), present_family.value()));
    }

    return std::nullopt;
}

[[nodiscard]] VkPhysicalDevice
choose_physical_device(VkInstance const instance, VkSurfaceKHR const surface, utility::slice<char const *> const device_extensions) noexcept
{
    auto count = uint32_t(0);
    vkEnumeratePhysicalDevices(instance, &count, nullptr);

    auto physical_devices = std::vector<VkPhysicalDevice>(count);
    vkEnumeratePhysicalDevices(instance, &count, std::data(physical_devices));

    auto const supported = [device_extensions, surface](auto const physical_device)
    {
        auto features = VkPhysicalDeviceFeatures();
        vkGetPhysicalDeviceFeatures(physical_device, &features);

        return check_extension_support(physical_device, device_extensions) && check_format_and_present_mode_availability(physical_device, surface) &&
               query_family_indices(physical_device, surface).has_value() && features.samplerAnisotropy;
    };

    auto const dev_start = std::begin(physical_devices);
    auto const dev_end   = std::end(physical_devices);
    auto const it        = std::find_if(dev_start, dev_end, supported);

    if (it != std::end(physical_devices))
    {
        return *it;
    }

    return nullptr;
}

} // namespace mvk::detail
