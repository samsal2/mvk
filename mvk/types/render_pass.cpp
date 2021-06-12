#include "types/render_pass.hpp"

namespace mvk::types
{

render_pass::render_pass(VkDevice const device,
                         VkRenderPassCreateInfo const & info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateRenderPass(parent(), &info, nullptr, &reference());

  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
