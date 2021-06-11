#include "vk_types/fence.hpp"

namespace mvk::vk_types
{

fence::fence(VkDevice const device, VkFenceCreateInfo const & create_info) : unique_wrapper_with_parent(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateFence(parent(), &create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}

fence &
fence::reset()
{
    [[maybe_unused]] auto const result = vkResetFences(parent(), 1, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
    return *this;
}

fence &
fence::wait()
{
    [[maybe_unused]] auto const result = vkWaitForFences(parent(), 1, &reference(), VK_TRUE, std::numeric_limits<int64_t>::max());
    MVK_VERIFY(VK_SUCCESS == result);
    return *this;
}

} // namespace mvk::vk_types
