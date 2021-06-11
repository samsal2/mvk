#include "vk_types/descriptor_pool.hpp"

namespace mvk::vk_types
{

descriptor_pool::descriptor_pool(VkDevice const device, VkDescriptorPoolCreateInfo const & create_info) : unique_wrapper_with_parent(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateDescriptorPool(parent(), &create_info, nullptr, &reference());

    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
