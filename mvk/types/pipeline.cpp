#include "types/pipeline.hpp"

namespace mvk::types
{

pipeline::pipeline(VkDevice const device,
                   VkGraphicsPipelineCreateInfo const & info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result = vkCreateGraphicsPipelines(
      parent(), nullptr, 1, &info, nullptr, &reference());
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
