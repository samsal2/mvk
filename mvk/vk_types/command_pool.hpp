#ifndef MVK_VK_TYPES_COMMAND_POOL_HPP_INCLUDED
#define MVK_VK_TYPES_COMMAND_POOL_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrapper.hpp"

namespace mvk::vk_types
{

class command_pool : public detail::wrapper<detail::deleter<vkDestroyCommandPool>, detail::handle<VkCommandPool>, detail::parent<VkDevice>>
{
public:
    constexpr command_pool() noexcept = default;
    command_pool(VkDevice device, VkCommandPoolCreateInfo const & create_info);

    void
    reset(VkCommandPoolResetFlags reset_flags) const;
};

} // namespace mvk::vk_types

#endif
