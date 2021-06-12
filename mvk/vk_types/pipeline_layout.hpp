#ifndef MVK_VK_TYPES_PIPELINE_LAYOUT_HPP_INCLUDED
#define MVK_VK_TYPES_PIPELINE_LAYOUT_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class pipeline_layout : public detail::wrapper<detail::deleter<vkDestroyPipelineLayout>, detail::handle<VkPipelineLayout>, detail::parent<VkDevice>>
{
public:
    constexpr pipeline_layout() noexcept = default;

    pipeline_layout(VkDevice device, VkPipelineLayoutCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif // MVK_VK_TYPES_PIPELINE_LAYOUT_HPP_INCLUDED
