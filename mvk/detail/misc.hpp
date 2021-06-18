#ifndef MVK_DETAIL_MISC_HPP_INCLUDED
#define MVK_DETAIL_MISC_HPP_INCLUDED

#include "types/types.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"
#include "vulkan/vulkan_core.h"

namespace mvk::detail
{

[[nodiscard]] bool
is_extension_present(
    std::string const & extension_name,
    utility::slice<VkExtensionProperties> extensions) noexcept;

[[nodiscard]] bool
check_extension_support(
    VkPhysicalDevice physical_device,
    utility::slice<char const *> device_extensions) noexcept;

[[nodiscard]] VkPhysicalDevice
choose_physical_device(
    VkInstance instance, VkSurfaceKHR surface,
    utility::slice<char const *> device_extensions) noexcept;

[[nodiscard]] constexpr bool
meets_graphic_requirements(
    VkQueueFamilyProperties const & queue_family) noexcept;

[[nodiscard]] bool
check_format_and_present_mode_availability(VkPhysicalDevice physical_device,
                                           VkSurfaceKHR surface) noexcept;

[[nodiscard]] bool
supports_surface(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                 uint32_t index);

[[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>>
query_family_indices(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

[[nodiscard]] uint32_t
choose_image_count(VkSurfaceCapabilitiesKHR const & capabilities) noexcept;

[[nodiscard]] VkPresentModeKHR
choose_present_mode(VkPhysicalDevice physical_device,
                    VkSurfaceKHR surface) noexcept;

[[nodiscard]] VkExtent2D
choose_extent(VkSurfaceCapabilitiesKHR const & capabilities,
              VkExtent2D const & extent) noexcept;

void
submit_draw_commands(types::device const & device,
                     types::single_command_buffer command_buffer,
                     types::semaphore const & image_available,
                     types::semaphore const & render_finished,
                     types::fence & frame_in_flight_fence) noexcept;

template <typename Checker>
requires types::detail::result_checker<Checker>
void
present_swapchain(types::device const & device,
                  types::swapchain const & swapchain,
                  types::semaphore const & render_finished,
                  uint32_t image_index, Checker && check) noexcept;

} // namespace mvk::detail

namespace mvk::detail
{
template <typename Checker>
requires types::detail::result_checker<Checker>
void
present_swapchain(types::device const & device,
                  types::swapchain const & swapchain,
                  types::semaphore const & render_finished,
                  uint32_t const image_index, Checker && check) noexcept
{
  auto const signal_semaphores = std::array{render_finished.get()};
  auto const swapchains = std::array{swapchain.get()};
  auto const image_indices = std::array{image_index};

  auto const present_info = [&signal_semaphores, &swapchains, &image_indices]
  {
    auto const [signal_semaphores_data, signal_semaphores_size] =
        utility::bind_data_and_size(signal_semaphores);

    auto const [swapchains_data, swapchains_size] =
        utility::bind_data_and_size(swapchains);

    auto const [image_indices_data, image_indices_size] =
        utility::bind_data_and_size(image_indices);

    auto info = VkPresentInfoKHR();
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = static_cast<uint32_t>(signal_semaphores_size);
    info.pWaitSemaphores = signal_semaphores_data;
    info.swapchainCount = static_cast<uint32_t>(swapchains_size);
    info.pSwapchains = swapchains_data;
    info.pImageIndices = image_indices_data;
    info.pResults = nullptr;
    return info;
  }();

  auto [graphics_queue, present_queue] = device.queues();
  present_queue.present(present_info, std::forward<Checker>(check))
      .wait_idle();
}

} // namespace mvk::detail

#endif
