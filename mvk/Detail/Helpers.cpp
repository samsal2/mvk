#include "Detail/Helpers.hpp"

#include "Detail/Misc.hpp"

#include <vector>

namespace Mvk::Detail {
[[nodiscard]] bool
isExtPresent(char const *ExtName,
             std::span<VkExtensionProperties const> const Exts) noexcept {
  for (auto const &Ext : Exts) {
    if (std::strcmp(Ext.extensionName, ExtName) == 0) {
      return true;
    }
  }

  return false;
}

[[nodiscard]] bool
chkExtSup(VkPhysicalDevice PhysicalDevice,
          std::span<char const *const> DeviceExtensions) noexcept {
  auto ExtCount = uint32_t(0);
  vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtCount,
                                       nullptr);

  auto Exts = std::vector<VkExtensionProperties>(ExtCount);
  vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtCount,
                                       std::data(Exts));

  for (auto const &DevExt : DeviceExtensions) {
    if (!isExtPresent(DevExt, Exts)) {
      return false;
    }
  }

  return true;
}

[[nodiscard]] bool
chkGfxReq(VkQueueFamilyProperties const &QueueFamily) noexcept {
  return (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U;
}

[[nodiscard]] bool
chkFmtAndPresentModeAvailablity(VkPhysicalDevice PhysicalDevice,
                                VkSurfaceKHR Surface) noexcept {
  auto FmtCount = uint32_t(0);
  vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FmtCount,
                                       nullptr);

  auto PresentModeCount = uint32_t(0);
  vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface,
                                            &PresentModeCount, nullptr);

  return FmtCount != 0 && PresentModeCount != 0;
}

[[nodiscard]] bool chSurfSup(VkPhysicalDevice PhysicalDevice,
                             VkSurfaceKHR Surface, uint32_t Idx) noexcept {
  auto Sup = VkBool32(false);
  vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, Idx, Surface, &Sup);

  return Sup != 0U;
}

[[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>>
queryFamiliyIdxs(VkPhysicalDevice const PhysicalDevice,
                 VkSurfaceKHR const Surface) {
  auto QueueFamilyCount = uint32_t(0);
  vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount,
                                           nullptr);

  auto QueueFamilyProps =
      std::vector<VkQueueFamilyProperties>(QueueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount,
                                           std::data(QueueFamilyProps));

  auto GfxFamily = std::optional<uint32_t>();
  auto PresentFamily = std::optional<uint32_t>();

  auto i = 0;
  for (auto const &QueueFamilyProp : QueueFamilyProps) {
    if (QueueFamilyProp.queueCount == 0) {
      continue;
    }

    if (chkGfxReq(QueueFamilyProp)) {
      GfxFamily = i;
    }

    if (chSurfSup(PhysicalDevice, Surface, i)) {
      PresentFamily = i;
    }

    if (PresentFamily.has_value() && GfxFamily.has_value()) {
      return std::make_optional(
          std::make_pair(GfxFamily.value(), PresentFamily.value()));
    }

    ++i;
  }

  return std::nullopt;
}

[[nodiscard]] uint32_t
chooseImgCount(VkSurfaceCapabilitiesKHR const &Capabilities) noexcept {
  auto const Candidate = Capabilities.minImageCount + 1;
  auto const Max = Capabilities.maxImageCount;

  if (Max > 0 && Candidate > Max) {
    return Max;
  }

  return Candidate;
}

[[nodiscard]] VkPresentModeKHR
choosePresentMode(VkPhysicalDevice PhysicalDevice,
                  VkSurfaceKHR Surface) noexcept {
  auto PresentModeCount = uint32_t(0);
  vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface,
                                            &PresentModeCount, nullptr);

  auto PresentModes = std::vector<VkPresentModeKHR>(PresentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      PhysicalDevice, Surface, &PresentModeCount, std::data(PresentModes));

  for (auto const PresentMode : PresentModes) {
    if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return PresentMode;
    }
  }

  return PresentModes[0];
}

[[nodiscard]] VkExtent2D
chooseExtent(VkSurfaceCapabilitiesKHR const &Capabilities,
             VkExtent2D const &Extent) noexcept {
  if (Capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return Capabilities.currentExtent;
  }

  auto const MinWidth = Capabilities.minImageExtent.width;
  auto const MinHeight = Capabilities.minImageExtent.height;
  auto const MaxWidth = Capabilities.maxImageExtent.width;
  auto const MaxHeight = Capabilities.maxImageExtent.height;

  auto const ExtentWidth = std::clamp(Extent.width, MinWidth, MaxWidth);
  auto const ExtentHeight = std::clamp(Extent.height, MinHeight, MaxHeight);

  return {ExtentWidth, ExtentHeight};
}

} // namespace Mvk::Detail
