#include "vk_types/device.hpp"

#include "utility/verify.hpp"

#include <_types/_uint32_t.h>

namespace mvk::vk_types
{

device::device(
  VkPhysicalDevice           physical_device,
  VkDeviceCreateInfo const & create_info)
  : wrapper(nullptr, make_deleter()),
    physical_device_(physical_device)
{
  [[maybe_unused]] auto const result =
    vkCreateDevice(physical_device_, &create_info, nullptr, &reference());

  MVK_VERIFY(VK_SUCCESS == result);

  auto const [graphics_index, present_index] = [&create_info]
  {
    auto const queue_count = create_info.queueCreateInfoCount;

    if (queue_count == 1)
    {
      auto const index = create_info.pQueueCreateInfos[0].queueFamilyIndex;
      return std::make_pair(index, index);
    }

    if (queue_count == 2)
    {
      auto const queues_info    = &create_info.pQueueCreateInfos[0];
      auto const graphics_index = queues_info[0].queueFamilyIndex;
      auto const present_index  = queues_info[1].queueFamilyIndex;
      return std::make_pair(graphics_index, present_index);
    }

    MVK_VERIFY_NOT_REACHED();
  }();

  queues_.graphics_queue_ = queue(get(), graphics_index);
  queues_.present_queue_  = queue(get(), present_index);
}

void
device::wait_idle() const noexcept
{
  vkDeviceWaitIdle(get());
}

} // namespace mvk::vk_types
