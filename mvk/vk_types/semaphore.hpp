#ifndef MVK_VK_TYPES_SEMAPHORE_HPP_INCLUDED
#define MVK_VK_TYPES_SEMAPHORE_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class semaphore : public detail::wrapper<VkSemaphore, vkDestroySemaphore>
{
public:
  constexpr semaphore() noexcept = default;
  semaphore(VkDevice device, VkSemaphoreCreateInfo const & create_info);
};

} // namespace mvk::vk_types

#endif
