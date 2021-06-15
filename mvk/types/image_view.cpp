#include "types/image_view.hpp"

namespace mvk::types
{

image_view::image_view(VkDevice const device,
                       VkImageViewCreateInfo const & info) noexcept
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateImageView(parent(), &info, nullptr, &get());

  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
