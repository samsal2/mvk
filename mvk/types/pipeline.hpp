#ifndef MVK_TYPES_PIPELINE_HPP_INCLUDED
#define MVK_TYPES_PIPELINE_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class pipeline : public detail::wrapper<detail::deleter<vkDestroyPipeline>,
                                        detail::handle<VkPipeline>,
                                        detail::parent<VkDevice>>
{
public:
  constexpr pipeline() noexcept = default;
  pipeline(VkDevice device, VkGraphicsPipelineCreateInfo const & info);
};

} // namespace mvk::types

#endif
