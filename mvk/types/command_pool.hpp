#ifndef MVK_TYPES_COMMAND_POOL_HPP_INCLUDED
#define MVK_TYPES_COMMAND_POOL_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class command_pool
    : public detail::wrapper<detail::deleter<vkDestroyCommandPool>,
                             detail::handle<VkCommandPool>,
                             detail::parent<VkDevice>>
{
public:
  constexpr command_pool() noexcept = default;

  command_pool(VkDevice device,
               VkCommandPoolCreateInfo const & info) noexcept;

  void
  reset(VkCommandPoolResetFlags reset_flags) const;
};

} // namespace mvk::types

#endif
