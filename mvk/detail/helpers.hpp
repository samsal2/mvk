#ifndef MVK_DETAIL_HELPERS_HPP_INCLUDED
#define MVK_DETAIL_HELPERS_HPP_INCLUDED

#include "Utility/Slice.hpp"

#include <optional>
#include <vulkan/vulkan.h>

namespace Mvk::Detail {
[[nodiscard]] bool
isExtPresent(char const *ExtName,
             Utility::Slice<VkExtensionProperties const> Exts) noexcept;

[[nodiscard]] bool
chkExtSup(VkPhysicalDevice PhysicalDevice,
          Utility::Slice<char const *const> DeviceExtensions) noexcept;

[[nodiscard]] bool
chkGfxReq(VkQueueFamilyProperties const &QueueFamily) noexcept;

[[nodiscard]] bool
chkFmtAndPresentModeAvailablity(VkPhysicalDevice PhysicalDevice,
                                VkSurfaceKHR Surface) noexcept;

[[nodiscard]] bool chSurfSup(VkPhysicalDevice PhysicalDevice,
                             VkSurfaceKHR Surface, uint32_t Idx) noexcept;

[[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>>
queryFamiliyIdxs(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);

[[nodiscard]] uint32_t
chooseImgCount(VkSurfaceCapabilitiesKHR const &Capabilities) noexcept;

[[nodiscard]] VkPresentModeKHR
choosePresentMode(VkPhysicalDevice PhysicalDevice,
                  VkSurfaceKHR Surface) noexcept;

[[nodiscard]] VkExtent2D
chooseExtent(VkSurfaceCapabilitiesKHR const &Capabilities,
             VkExtent2D const &Extent) noexcept;

} // namespace Mvk::Detail

#endif
