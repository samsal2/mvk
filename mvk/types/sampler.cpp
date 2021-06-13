#include "types/sampler.hpp"

namespace mvk::types
{

sampler::sampler(VkDevice const device, VkSamplerCreateInfo const & info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateSampler(parent(), &info, nullptr, &get());
  MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::types
