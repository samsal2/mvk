#include "types/queue.hpp"

#include "types/device.hpp"
#include "types/fence.hpp"

namespace mvk::types
{

queue::queue(VkDevice const device, uint32_t const index) noexcept
    : index_(index)
{
  vkGetDeviceQueue(device, index, 0, &instance_);
}

queue &
queue::wait_idle() noexcept
{
  [[maybe_unused]] auto const result = vkQueueWaitIdle(instance_);
  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

queue &
queue::submit(VkSubmitInfo const & info, VkFence const fence) noexcept
{
  [[maybe_unused]] auto const result = vkQueueSubmit(get(), 1, &info, fence);
  MVK_VERIFY(VK_SUCCESS == result);
  return *this;
}

} // namespace mvk::types
