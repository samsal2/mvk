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
  [[nodiscard]] std::tuple<std::vector<unsigned char>, uint32_t, uint32_t> loadTex(std::filesystem::path const & Path)
  {
    MVK_VERIFY(std::filesystem::exists(Path));

    auto       Width    = 0;
    auto       Height   = 0;
    auto       Channels = 0;
    auto const Pixels   = stbi_load(Path.c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);

    auto Buff =
      std::vector<unsigned char>(static_cast<uint32_t>(Width) * static_cast<uint32_t>(Height) * 4 * sizeof(*Pixels));
    std::copy(Pixels, std::next(Pixels, static_cast<int64_t>(std::size(Buff))), std::begin(Buff));

    stbi_image_free(Pixels);

    return { std::move(Buff), Width, Height };
  }

  [[nodiscard]] std::optional<uint32_t>
    queryMemType(VkPhysicalDevice PhysicalDevice, uint32_t Filter, VkMemoryPropertyFlags PropFlags)
  {
    auto MemProp = VkPhysicalDeviceMemoryProperties();
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemProp);

    auto const TypeCnt = MemProp.memoryTypeCount;

    for (auto i = uint32_t(0); i < TypeCnt; ++i)
    {
      auto const & CurrentType   = MemProp.memoryTypes[i];
      auto const   CurrentFlags  = CurrentType.propertyFlags;
      auto const   MatchesFlags  = (CurrentFlags & PropFlags) != 0U;
      auto const   MatchesFilter = (Filter & (1U << i)) != 0U;

      if (MatchesFlags && MatchesFilter)
      {
        return i;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint32_t> querySwapchainImg(VkDevice const       Device,
                                                          VkSwapchainKHR const Swapchain,
                                                          VkSemaphore const    Semaphore,
                                                          VkFence const        Fence)
  {
    auto Idx = uint32_t(0);

    auto const Result =
      vkAcquireNextImageKHR(Device, Swapchain, std::numeric_limits<uint64_t>::max(), Semaphore, Fence, &Idx);

    if (Result != VK_ERROR_OUT_OF_DATE_KHR)
    {
      return Idx;
    }

    return std::nullopt;
  }

  [[nodiscard]] uint32_t calcMipLvl(uint32_t const Height, uint32_t const Width) noexcept
  {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(Height, Width))) + 1);
  }

  [[nodiscard]] utility::Slice<std::byte>
    map_memory(VkDevice Device, VkDeviceMemory Mem, VkDeviceSize Size, VkDeviceSize Off) noexcept
  {
    void * Data = nullptr;
    vkMapMemory(Device, Mem, Off, Size, 0, &Data);
    return { utility::forceCastToByte(Data), Size };
  }
}  // namespace mvk::detail
