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

  fence(VkDevice device, VkFenceCreateInfo const & info);

  fence &
  reset();
  fence &
  wait();
};

} // namespace mvk::types

#endif
