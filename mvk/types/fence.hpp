#ifndef MVK_TYPES_FENCE_HPP_INCLUDED
#define MVK_TYPES_FENCE_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class fence : public detail::wrapper<detail::deleter<vkDestroyFence>,
                                     detail::handle<VkFence>,
                                     detail::parent<VkDevice>>
{
public:
  constexpr fence() noexcept = default;

  fence(VkDevice device, VkFenceCreateInfo const & info) noexcept;

  fence &
  reset() noexcept;
  fence &
  wait() noexcept;
};

} // namespace mvk::types

#endif
