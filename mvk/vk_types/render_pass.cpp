#include "vk_types/render_pass.hpp"

namespace mvk::vk_types
{

render_pass::render_pass(VkDevice const device, VkRenderPassCreateInfo const & create_info) : unique_wrapper_with_parent(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateRenderPass(parent(), &create_info, nullptr, &reference());

    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
