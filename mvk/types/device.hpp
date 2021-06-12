#ifndef MVK_TYPES_DEVICE_HPP_INCLUDED
#define MVK_TYPES_DEVICE_HPP_INCLUDED

#include "types/common.hpp"
#include "types/detail/wrapper.hpp"
#include "types/queue.hpp"

namespace mvk::types
{

class device : public detail::wrapper<detail::deleter<vkDestroyDevice>,
                                      detail::handle<VkDevice>>
{
public:
  struct queues
  {
    queue graphics_queue_;
    queue present_queue_;
  };

  constexpr device() noexcept = default;

  device(VkPhysicalDevice physical_device, VkDeviceCreateInfo const & info);

  [[nodiscard]] constexpr queues
  get_queues() const noexcept;

  [[nodiscard]] constexpr VkPhysicalDevice
  physical_device() const noexcept;

  void
  wait_idle() const noexcept;

private:
  VkPhysicalDevice physical_device_ = nullptr;
  queues queues_;
};

[[nodiscard]] constexpr device::queues
device::get_queues() const noexcept
{
  return queues_;
}

[[nodiscard]] constexpr VkPhysicalDevice
device::physical_device() const noexcept
{
  return physical_device_;
}

} // namespace mvk::types

#endif
