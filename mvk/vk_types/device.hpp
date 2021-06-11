#ifndef MVK_VK_TYPES_DEVICE_HPP_INCLUDED
#define MVK_VK_TYPES_DEVICE_HPP_INCLUDED

#include "vk_types/common.hpp"
#include "vk_types/detail/wrappers.hpp"
#include "vk_types/queue.hpp"

namespace mvk::vk_types
{

class device : public detail::unique_wrapper<VkDevice, vkDestroyDevice>
{
public:
    struct queues
    {
        queue graphics_queue_;
        queue present_queue_;
    };

    constexpr device() noexcept = default;

    device(VkPhysicalDevice physical_device, VkDeviceCreateInfo const & create_info);

    [[nodiscard]] constexpr queues
    get_queues() const noexcept;

    [[nodiscard]] constexpr VkPhysicalDevice
    physical_device() const noexcept;

    void
    wait_idle() const noexcept;

private:
    VkPhysicalDevice physical_device_ = nullptr;
    queues           queues_;
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

} // namespace mvk::vk_types

#endif
