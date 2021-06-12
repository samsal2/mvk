#include "types/image_view.hpp"

namespace mvk::types
{

image_view::image_view(VkDevice const device,
                       VkImageViewCreateInfo const & create_info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateImageView(parent(), &create_info, nullptr, &reference());

  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
