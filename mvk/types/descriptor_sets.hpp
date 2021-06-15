#ifndef MVK_TYPES_DESCRIPTOR_SETS_HPP_INCLUDED
#define MVK_TYPES_DESCRIPTOR_SETS_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "utility/slice.hpp"

namespace mvk::types
{

class descriptor_sets
    : public detail::wrapper<detail::deleter<vkFreeDescriptorSets>,
                             detail::handle<std::vector<VkDescriptorSet>>,
                             detail::parent<VkDevice>,
                             detail::pool<VkDescriptorPool>>
{
public:
  constexpr descriptor_sets() noexcept = default;

  descriptor_sets(VkDevice device,
                  VkDescriptorSetAllocateInfo const & info) noexcept;
};

void
update_descriptor_sets(VkDevice device,
                       utility::slice<VkWriteDescriptorSet> descriptor_writes,
                       utility::slice<VkCopyDescriptorSet> descriptor_copies);

} // namespace mvk::types

#endif
