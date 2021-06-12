#ifndef MVK_TYPES_SEMAPHORE_HPP_INCLUDED
#define MVK_TYPES_SEMAPHORE_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"

namespace mvk::types
{

class semaphore : public detail::wrapper<detail::deleter<vkDestroySemaphore>,
                                         detail::handle<VkSemaphore>,
                                         detail::parent<VkDevice>>
{
public:
  constexpr semaphore() noexcept = default;

  semaphore(VkDevice device, VkSemaphoreCreateInfo const & info);
};

} // namespace mvk::types

#endif
