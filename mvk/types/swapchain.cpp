#include "types/swapchain.hpp"

#include "types/common.hpp"
#include "types/detail/misc.hpp"
#include "types/fence.hpp"
#include "types/semaphore.hpp"

namespace mvk::types
{

swapchain::swapchain(VkDevice const device,
                     VkSwapchainCreateInfoKHR const & info)
    : wrapper(nullptr, device), extent_(info.imageExtent),
      format_(info.imageFormat)
{
  [[maybe_unused]] auto const result =
      vkCreateSwapchainKHR(parent(), &info, nullptr, &reference());

  MVK_VERIFY(VK_SUCCESS == result);

  auto images_size = uint32_t(0);
  vkGetSwapchainImagesKHR(parent(), get(), &images_size, nullptr);

  images_.resize(images_size);
  vkGetSwapchainImagesKHR(parent(), get(), &images_size, std::data(images_));

  image_views_.reserve(images_size);

  auto const add_image_view = [this](auto const image)
  {
    auto const image_view_create_info = [this, image]
    {
      auto view_info = VkImageViewCreateInfo();
      view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_info.image = image;
      view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_info.format = format_;
      view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      view_info.subresourceRange.baseMipLevel = 0;
      view_info.subresourceRange.levelCount = 1;
      view_info.subresourceRange.baseArrayLayer = 0;
      view_info.subresourceRange.layerCount = 1;
      return view_info;
    }();

    image_views_.emplace_back(parent(), image_view_create_info);
  };

  std::for_each(std::begin(images_), std::end(images_), add_image_view);
}

[[nodiscard]] std::optional<uint32_t>
swapchain::next_image(VkSemaphore const semaphore,
                      VkFence const fence) const noexcept
{
  return detail::next_swapchain_image(parent(), get(), semaphore, fence);
}

} // namespace mvk::types
