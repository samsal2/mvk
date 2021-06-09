#include "vk_types/descriptor_sets.hpp"

namespace mvk::vk_types
{

descriptor_sets::descriptor_sets(
  VkDevice const                      device,
  VkDescriptorSetAllocateInfo const & allocate_info)
  : wrapper(
      std::vector<VkDescriptorSet>(allocate_info.descriptorSetCount),
      make_deleter(device, allocate_info.descriptorPool))
{
  [[maybe_unused]] auto const result =
    vkAllocateDescriptorSets(parent(), &allocate_info, std::data(reference()));

  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
