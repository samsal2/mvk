#include "vk_types/pipeline.hpp"

namespace mvk::vk_types
{

pipeline::pipeline(VkDevice const device, VkGraphicsPipelineCreateInfo const & create_info) : unique_wrapper_with_parent(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateGraphicsPipelines(parent(), nullptr, 1, &create_info, nullptr, &reference());

    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
