#ifndef MVK_TYPES_DESCRIPTOR_POOL_HPP_INCLUDED
#define MVK_TYPES_DESCRIPTOR_POOL_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class descriptor_pool
    : public detail::wrapper<detail::deleter<vkDestroyDescriptorPool>,
                             detail::handle<VkDescriptorPool>,
                             detail::parent<VkDevice>>
{
public:
  constexpr descriptor_pool() noexcept = default;

  descriptor_pool(VkDevice device, VkDescriptorPoolCreateInfo const & info);
};

} // namespace mvk::types

#endif
