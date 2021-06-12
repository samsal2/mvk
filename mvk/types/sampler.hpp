#ifndef MVK_TYPES_SAMPLER_HPP_INCLUDE
#define MVK_TYPES_SAMPLER_HPP_INCLUDE

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class sampler : public detail::wrapper<detail::deleter<vkDestroySampler>,
                                       detail::handle<VkSampler>,
                                       detail::parent<VkDevice>>
{
public:
  constexpr sampler() noexcept = default;

  sampler(VkDevice device, VkSamplerCreateInfo const & info);
};

} // namespace mvk::types

#endif
