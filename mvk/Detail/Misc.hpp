#pragma once

#include "Utility/Verify.hpp"

#include <filesystem>
#include <optional>
#include <span>
#include <vulkan/vulkan.h>

namespace Mvk::Detail {
[[nodiscard]] std::tuple<std::vector<unsigned char>, uint32_t, uint32_t>
loadTex(std::filesystem::path const &Path);

[[nodiscard]] std::optional<uint32_t>
queryMemType(VkPhysicalDevice PhysicalDevice, uint32_t Filter,
             VkMemoryPropertyFlags PropFlags);

[[nodiscard]] std::optional<uint32_t>
querySwapchainImg(VkDevice Device, VkSwapchainKHR Swapchain,
                  VkSemaphore Semaphore, VkFence Fence);

[[nodiscard]] uint32_t calcMipLvl(uint32_t Height, uint32_t Width) noexcept;

[[nodiscard]] constexpr auto alignedSize(size_t Size,
                                         size_t Alignment) noexcept {
  if (auto Mod = Size % Alignment; Mod != 0) {
    return Size + Alignment - Mod;
  }

  return static_cast<decltype(Size + Alignment)>(Size);
}

} // namespace Mvk::Detail

