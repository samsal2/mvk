#pragma once

#include <optional>
#include <span>
#include <vulkan/vulkan.h>

namespace Mvk::Detail
{
  [[nodiscard]] bool isExtPresent( char const * ExtName, std::span<VkExtensionProperties const> Exts ) noexcept;

  [[nodiscard]] bool chkExtSup( VkPhysicalDevice PhysicalDevice, std::span<char const * const> DeviceExtensions ) noexcept;

  [[nodiscard]] bool chkGfxReq( VkQueueFamilyProperties const & QueueFamily ) noexcept;

  [[nodiscard]] bool chkFmtAndPresentModeAvailablity( VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface ) noexcept;

  [[nodiscard]] bool chSurfSup( VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, uint32_t Idx ) noexcept;

  [[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>> queryFamiliyIdxs( VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface );

  [[nodiscard]] uint32_t chooseImgCount( VkSurfaceCapabilitiesKHR const & Capabilities ) noexcept;

  [[nodiscard]] VkPresentModeKHR choosePresentMode( VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface ) noexcept;

  [[nodiscard]] VkExtent2D chooseExtent( VkSurfaceCapabilitiesKHR const & Capabilities, VkExtent2D const & Extent ) noexcept;

}  // namespace Mvk::Detail
