#ifndef MVK_VK_TYPES_RENDER_PASS_HPP_INCLUDED
#define MVK_VK_TYPES_RENDER_PASS_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class render_pass : public detail::wrapper<VkRenderPass, vkDestroyRenderPass>
{
public:
  constexpr render_pass() noexcept = default;
  render_pass(VkDevice device, VkRenderPassCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
