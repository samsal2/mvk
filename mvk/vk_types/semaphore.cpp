#include "vk_types/semaphore.hpp"

namespace mvk::vk_types
{

semaphore::semaphore(VkDevice const device, VkSemaphoreCreateInfo const & create_info) : wrapper(nullptr, device)
{
    [[maybe_unused]] auto const result = vkCreateSemaphore(parent(), &create_info, nullptr, &reference());
    MVK_VERIFY(VK_SUCCESS == result);
}
} // namespace mvk::vk_types
