#include "vk_types/sampler.hpp"

namespace mvk::vk_types
{

sampler::sampler(VkDevice const device, VkSamplerCreateInfo const & create_info) : wrapper(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateSampler(parent(), &create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
