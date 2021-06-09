#include "vk_types/framebuffer.hpp"

namespace mvk::vk_types
{

framebuffer::framebuffer(
  VkDevice const                  device,
  VkFramebufferCreateInfo const & create_info)
  : wrapper(nullptr, make_deleter(device))
{
  [[maybe_unused]] auto const result =
    vkCreateFramebuffer(parent(), &create_info, nullptr, &reference());

  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
