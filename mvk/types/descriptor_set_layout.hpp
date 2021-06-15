#ifndef MVK_TYPES_DESCRIPTOR_SET_LAYOUT_HPP_INCLUDED
#define MVK_TYPES_DESCRIPTOR_SET_LAYOUT_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class descriptor_set_layout
    : public detail::wrapper<detail::deleter<vkDestroyDescriptorSetLayout>,
                             detail::handle<VkDescriptorSetLayout>,
                             detail::parent<VkDevice>>
{
public:
  constexpr descriptor_set_layout() noexcept = default;

  descriptor_set_layout(
      VkDevice device,
      VkDescriptorSetLayoutCreateInfo const & create_info) noexcept;
};

} // namespace mvk::types

#endif
