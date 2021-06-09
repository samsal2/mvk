#include "vk_types/swapchain.hpp"

#include "vk_types/common.hpp"
#include "vk_types/detail/misc.hpp"
#include "vk_types/fence.hpp"
#include "vk_types/semaphore.hpp"
#include "vulkan/vulkan_core.h"

namespace mvk::vk_types
{

swapchain::swapchain(
  VkDevice const                   device,
  VkSwapchainCreateInfoKHR const & create_info)
  : wrapper(nullptr, make_deleter(device)),
    extent_(create_info.imageExtent)
{
  [[maybe_unused]] auto const result =
    vkCreateSwapchainKHR(parent(), &create_info, nullptr, &reference());

  MVK_VERIFY(VK_SUCCESS == result);

  auto images_size = uint32_t(0);
  vkGetSwapchainImagesKHR(parent(), get(), &images_size, nullptr);

  images_.resize(images_size);
  vkGetSwapchainImagesKHR(parent(), get(), &images_size, std::data(images_));
}

[[nodiscard]] std::optional<uint32_t>
swapchain::next_image(VkSemaphore const semaphore, VkFence const fence)
  const noexcept
{
  return detail::next_swapchain_image(parent(), get(), semaphore, fence);
}

} // namespace mvk::vk_types
