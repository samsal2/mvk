#ifndef MVK_TYPES_PIPELINE_LAYOUT_HPP_INCLUDED
#define MVK_TYPES_PIPELINE_LAYOUT_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class pipeline_layout
    : public detail::wrapper<detail::deleter<vkDestroyPipelineLayout>,
                             detail::handle<VkPipelineLayout>,
                             detail::parent<VkDevice>>
{
public:
  constexpr pipeline_layout() noexcept = default;

  pipeline_layout(VkDevice device, VkPipelineLayoutCreateInfo const & info);
};

} // namespace mvk::types

#endif
