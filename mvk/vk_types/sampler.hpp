#ifndef MVK_VK_TYPES_SAMPLER_HPP_INCLUDE
#define MVK_VK_TYPES_SAMPLER_HPP_INCLUDE

#include "vk_types/common.hpp"
#include "vk_types/detail/wrappers.hpp"

namespace mvk::vk_types
{

class sampler : public detail::unique_wrapper_with_parent<VkSampler, VkDevice, vkDestroySampler>
{
public:
    constexpr sampler() noexcept = default;
    sampler(VkDevice device, VkSamplerCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
