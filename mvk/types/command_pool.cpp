#include "types/command_pool.hpp"

namespace mvk::types
{

command_pool::command_pool(VkDevice const device, VkCommandPoolCreateInfo const & create_info) : wrapper(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateCommandPool(parent(), &create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

void
command_pool::reset(VkCommandPoolResetFlags const reset_flags) const
{
    [[maybe_unused]] auto const result = vkResetCommandPool(parent(), get(), reset_flags);
    MVK_VERIFY(VK_SUCCESS == result);
}
} // namespace mvk::types