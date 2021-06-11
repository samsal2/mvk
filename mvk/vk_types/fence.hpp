#ifndef MVK_VK_TYPES_FENCE_HPP_INCLUDED
#define MVK_VK_TYPES_FENCE_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrappers.hpp"

namespace mvk::vk_types
{

class fence : public detail::unique_wrapper_with_parent<VkFence, VkDevice, vkDestroyFence>
{
public:
    constexpr fence() noexcept = default;
    fence(VkDevice device, VkFenceCreateInfo const & create_info);

    fence &
    reset();
    fence &
    wait();
};

} // namespace mvk::vk_types

#endif
