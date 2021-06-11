#include "vk_types/descriptor_set_layout.hpp"

namespace mvk::vk_types
{

descriptor_set_layout::descriptor_set_layout(VkDevice const device, VkDescriptorSetLayoutCreateInfo const & create_info)
    : unique_wrapper_with_parent(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateDescriptorSetLayout(parent(), &create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
