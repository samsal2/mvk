#ifndef MVK_VK_TYPES_DESCRIPTOR_SET_LAYOUT_HPP_INCLUDED
#define MVK_VK_TYPES_DESCRIPTOR_SET_LAYOUT_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class descriptor_set_layout
  : public detail::wrapper<VkDescriptorSetLayout, vkDestroyDescriptorSetLayout>
{
public:
  constexpr descriptor_set_layout() noexcept = default;
  descriptor_set_layout(
    VkDevice                                device,
    VkDescriptorSetLayoutCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
