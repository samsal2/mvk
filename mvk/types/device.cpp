#include "types/device.hpp"

#include "utility/verify.hpp"

#include <_types/_uint32_t.h>

namespace mvk::types
{

device::device(VkPhysicalDevice physical_device,
               VkDeviceCreateInfo const & info)
    : wrapper(), physical_device_(physical_device)
{
  [[maybe_unused]] auto const result =
      vkCreateDevice(physical_device_, &info, nullptr, &get());

  MVK_VERIFY(VK_SUCCESS == result);

  auto const [graphics_index, present_index] = [&info]
  {
    auto const queue_count = info.queueCreateInfoCount;

    if (queue_count == 1)
    {
      auto const index = info.pQueueCreateInfos[0].queueFamilyIndex;
      return std::make_pair(index, index);
    }

    if (queue_count == 2)
    {
      auto const queues_info = &info.pQueueCreateInfos[0];
      auto const graphics_index = queues_info[0].queueFamilyIndex;
      auto const present_index = queues_info[1].queueFamilyIndex;
      return std::make_pair(graphics_index, present_index);
    }

    MVK_VERIFY_NOT_REACHED();
  }();

  queues_.graphics_queue_ = queue(get(), graphics_index);
  queues_.present_queue_ = queue(get(), present_index);
}

void
device::wait_idle() const noexcept
{
  vkDeviceWaitIdle(get());
}

} // namespace mvk::types
