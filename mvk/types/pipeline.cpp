#include "types/pipeline.hpp"

namespace mvk::types
{

pipeline::pipeline(VkDevice const device, VkGraphicsPipelineCreateInfo const & create_info) : wrapper(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateGraphicsPipelines(parent(), nullptr, 1, &create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
