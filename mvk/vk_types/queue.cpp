#include "vk_types/queue.hpp"

#include "vk_types/device.hpp"
#include "vk_types/fence.hpp"

namespace mvk::vk_types
{

queue::queue(VkDevice const device, uint32_t const index) : index_(index)
{
  vkGetDeviceQueue(device, index, 0, &instance_);
}

queue &
queue::wait_idle() noexcept
{
  vkQueueWaitIdle(instance_);
  return *this;
}

queue &
queue::submit(VkSubmitInfo const & submit_info, fence const & fence)
{
  [[maybe_unused]] auto const result =
    vkQueueSubmit(get(), 1, &submit_info, fence.get());
  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

queue &
queue::submit(VkSubmitInfo const & submit_info)
{
  [[maybe_unused]] auto const result =
    vkQueueSubmit(get(), 1, &submit_info, nullptr);
  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

} // namespace mvk::vk_types
