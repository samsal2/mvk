#ifndef MVK_VK_TYPES_SWAPCHAIN_HPP_INCLUDED
#define MVK_VK_TYPES_SWAPCHAIN_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

#include <optional>
#include <vector>

namespace mvk::vk_types
{

class semaphore;
class fence;

class swapchain : public detail::wrapper<VkSwapchainKHR, vkDestroySwapchainKHR>
{
public:
  swapchain() noexcept = default;

  swapchain(VkDevice device, VkSwapchainCreateInfoKHR const & create_info);

  [[nodiscard]] constexpr VkExtent2D
  extent() const noexcept;

  [[nodiscard]] constexpr std::vector<VkImage> const &
  images() const noexcept;

  [[nodiscard]] std::optional<uint32_t>
  next_image(VkSemaphore semaphore, VkFence fence = nullptr) const noexcept;

private:
  std::vector<VkImage> images_;
  VkExtent2D           extent_ = {};
};

[[nodiscard]] constexpr std::vector<VkImage> const &
swapchain::images() const noexcept
{
  return images_;
}

[[nodiscard]] constexpr VkExtent2D
swapchain::extent() const noexcept
{
  return extent_;
}

} // namespace mvk::vk_types

#endif // MVK_VK_TYPES_SWAPCHAIN_HPP_INCLUDED
