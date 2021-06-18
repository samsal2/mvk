#include "types/descriptor_sets.hpp"

namespace mvk::types
{

descriptor_sets::descriptor_sets(
    VkDevice const device, VkDescriptorSetAllocateInfo const & info) noexcept
    : wrapper(std::vector<VkDescriptorSet>(info.descriptorSetCount, nullptr),
              device, info.descriptorPool)
{
  [[maybe_unused]] auto const result =
      vkAllocateDescriptorSets(parent(), &info, std::data(get()));

  MVK_VERIFY(VK_SUCCESS == result);
}

void
update_descriptor_sets(
    VkDevice const device,
    utility::slice<VkWriteDescriptorSet> const descriptor_writes,
    utility::slice<VkCopyDescriptorSet> const descriptor_copies)
{
  auto const [descriptor_writes_data, descriptor_writes_size] =
      utility::bind_data_and_size(descriptor_writes);
  auto const [descriptor_copies_data, descriptor_copies_size] =
      utility::bind_data_and_size(descriptor_copies);

  vkUpdateDescriptorSets(
      device, static_cast<uint32_t>(descriptor_writes_size),
      descriptor_writes_data, static_cast<uint32_t>(descriptor_copies_size),
      descriptor_copies_data);
}

} // namespace mvk::types
