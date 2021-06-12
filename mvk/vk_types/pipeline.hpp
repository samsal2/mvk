#ifndef MVK_VK_TYPES_PIPELINE_HPP_INCLUDED
#define MVK_VK_TYPES_PIPELINE_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class pipeline : public detail::wrapper<detail::deleter<vkDestroyPipeline>, detail::handle<VkPipeline>, detail::parent<VkDevice>>
{
public:
    constexpr pipeline() noexcept = default;
    pipeline(VkDevice device, VkGraphicsPipelineCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
