#include "types/framebuffer.hpp"

namespace mvk::types
{

framebuffer::framebuffer(VkDevice const device,
                         VkFramebufferCreateInfo const & info) noexcept
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateFramebuffer(parent(), &info, nullptr, &get());
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
