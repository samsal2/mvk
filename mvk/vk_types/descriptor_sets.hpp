#ifndef MVK_VK_TYPES_DESCRIPTOR_SETS_HPP_INCLUDED
#define MVK_VK_TYPES_DESCRIPTOR_SETS_HPP_INCLUDED

#include "utility/slice.hpp"
#include "vk_types/common.hpp"
#include "vk_types/detail/wrappers.hpp"

namespace mvk::vk_types
{

class descriptor_sets : public detail::unique_wrapper_with_parent_and_pool_allocated<VkDescriptorSet, VkDevice, VkDescriptorPool, vkFreeDescriptorSets>
{
public:
    constexpr descriptor_sets() noexcept = default;

    descriptor_sets(VkDevice device, VkDescriptorSetAllocateInfo const & allocate_info);
};

void
update_descriptor_sets(VkDevice device, utility::slice<VkWriteDescriptorSet> descriptor_writes, utility::slice<VkCopyDescriptorSet> descriptor_copies);

} // namespace mvk::vk_types

#endif
