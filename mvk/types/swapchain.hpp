#ifndef MVK_TYPES_SWAPCHAIN_HPP_INCLUDED
#define MVK_TYPES_SWAPCHAIN_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "types/image_view.hpp"
#include "utility/slice.hpp"

#include <optional>
#include <vector>

namespace mvk::types
{

class swapchain
    : public detail::wrapper<detail::deleter<vkDestroySwapchainKHR>,
                             detail::handle<VkSwapchainKHR>,
                             detail::parent<VkDevice>>
{
public:
  swapchain() noexcept = default;

  swapchain(VkDevice device, VkSwapchainCreateInfoKHR const & info);

  [[nodiscard]] constexpr VkExtent2D
  extent() const noexcept;

  [[nodiscard]] constexpr VkFormat
  format() const noexcept;

  [[nodiscard]] constexpr utility::slice<VkImage>
  images() const noexcept;

  [[nodiscard]] constexpr utility::slice<image_view>
  image_views() const noexcept;

  [[nodiscard]] std::optional<uint32_t>
  next_image(VkSemaphore semaphore, VkFence fence = nullptr) const noexcept;

private:
  std::vector<VkImage> images_;
  std::vector<image_view> image_views_;
  VkExtent2D extent_ = {};
  VkFormat format_ = {};
};

[[nodiscard]] constexpr utility::slice<VkImage>
swapchain::images() const noexcept
{
  return {images_};
}

[[nodiscard]] constexpr utility::slice<image_view>
swapchain::image_views() const noexcept
{
  return {image_views_};
}

[[nodiscard]] constexpr VkExtent2D
swapchain::extent() const noexcept
{
  return extent_;
}

[[nodiscard]] constexpr VkFormat
swapchain::format() const noexcept
{
  return format_;
}

} // namespace mvk::types

#endif // MVK_TYPES_SWAPCHAIN_HPP_INCLUDED
