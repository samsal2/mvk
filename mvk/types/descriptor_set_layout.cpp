#include "types/descriptor_set_layout.hpp"

namespace mvk::types
{

descriptor_set_layout::descriptor_set_layout(
    VkDevice const device, VkDescriptorSetLayoutCreateInfo const & info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateDescriptorSetLayout(parent(), &info, nullptr, &reference());
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
