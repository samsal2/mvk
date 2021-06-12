#include "vk_types/pipeline_layout.hpp"

namespace mvk::vk_types
{

pipeline_layout::pipeline_layout(VkDevice const device, VkPipelineLayoutCreateInfo const & create_info) : wrapper(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreatePipelineLayout(parent(), &create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
