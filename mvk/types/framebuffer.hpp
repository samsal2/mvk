#ifndef MVK_TYPES_FRAMEBUFFER_HPP_INCLUDED
#define MVK_TYPES_FRAMEBUFFER_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class framebuffer
    : public detail::wrapper<detail::deleter<vkDestroyFramebuffer>,
                             detail::handle<VkFramebuffer>,
                             detail::parent<VkDevice>>
{
public:
  constexpr framebuffer() noexcept = default;

  framebuffer(VkDevice device, VkFramebufferCreateInfo const & info);
};

} // namespace mvk::types

#endif
