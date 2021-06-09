#ifndef MVK_VK_TYPES_FRAMEBUFFER_HPP_INCLUDED
#define MVK_VK_TYPES_FRAMEBUFFER_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class framebuffer : public detail::wrapper<VkFramebuffer, vkDestroyFramebuffer>
{
public:
  constexpr framebuffer() noexcept = default;
  framebuffer(VkDevice device, VkFramebufferCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
