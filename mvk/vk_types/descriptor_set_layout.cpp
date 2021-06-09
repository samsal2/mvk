#include "vk_types/descriptor_set_layout.hpp"

namespace mvk::vk_types
{

descriptor_set_layout::descriptor_set_layout(
  VkDevice const                          device,
  VkDescriptorSetLayoutCreateInfo const & create_info)
  : wrapper(nullptr, make_deleter(device))
{
  [[maybe_unused]] auto const result =
    vkCreateDescriptorSetLayout(parent(), &create_info, nullptr, &reference());
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types