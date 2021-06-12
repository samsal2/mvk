#ifndef MVK_TYPES_INSTANCE_HPP_INCLUDED
#define MVK_TYPES_INSTANCE_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class instance : public detail::wrapper<detail::deleter<vkDestroyInstance>,
                                        detail::handle<VkInstance>>
{
public:
  constexpr instance() noexcept = default;

  explicit instance(VkInstanceCreateInfo const & create_info);
};

} // namespace mvk::types

#endif // MVK_TYPES_INSTANCE_HPP_INCLUDED
