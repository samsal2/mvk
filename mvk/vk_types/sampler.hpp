#ifndef MVK_VK_TYPES_SAMPLER_HPP_INCLUDE
#define MVK_VK_TYPES_SAMPLER_HPP_INCLUDE

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class sampler : public detail::wrapper<VkSampler, vkDestroySampler>
{
public:
        constexpr sampler() noexcept = default;
        sampler(VkDevice device, VkSamplerCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
