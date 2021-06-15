#ifndef MVK_TYPES_RENDER_PASS_HPP_INCLUDED
#define MVK_TYPES_RENDER_PASS_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class render_pass
    : public detail::wrapper<detail::deleter<vkDestroyRenderPass>,
                             detail::handle<VkRenderPass>,
                             detail::parent<VkDevice>>
{
public:
  constexpr render_pass() noexcept = default;

  render_pass(VkDevice device, VkRenderPassCreateInfo const & info) noexcept;
};

} // namespace mvk::types

#endif
