#ifndef MVK_VK_TYPES_DESCRIPTOR_POOL_HPP_INCLUDED
#define MVK_VK_TYPES_DESCRIPTOR_POOL_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrappers.hpp"

namespace mvk::vk_types
{

class descriptor_pool : public detail::unique_wrapper_with_parent<VkDescriptorPool, VkDevice, vkDestroyDescriptorPool>
{
public:
    constexpr descriptor_pool() noexcept = default;
    descriptor_pool(VkDevice device, VkDescriptorPoolCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
