#include "types/pipeline_layout.hpp"

namespace mvk::types
{

pipeline_layout::pipeline_layout(VkDevice const device,
                                 VkPipelineLayoutCreateInfo const & info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreatePipelineLayout(parent(), &info, nullptr, &get());
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
