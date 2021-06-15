#include "types/pipeline.hpp"

namespace mvk::types
{

pipeline::pipeline(VkDevice const device,
                   VkGraphicsPipelineCreateInfo const & info) noexcept
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateGraphicsPipelines(parent(), nullptr, 1, &info, nullptr, &get());
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
