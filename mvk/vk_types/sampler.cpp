#include "vk_types/sampler.hpp"

namespace mvk::vk_types
{

sampler::sampler(VkDevice const device, VkSamplerCreateInfo const & create_info) : unique_wrapper_with_parent(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateSampler(parent(), &create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

} // namespace mvk::vk_types
