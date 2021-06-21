#ifndef MVK_DETAIL_MISC_HPP_INCLUDED
#define MVK_DETAIL_MISC_HPP_INCLUDED

#include "detail/checkers.hpp"
#include "detail/query.hpp"
#include "types/types.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"

#include <filesystem>
#include <optional>
#include <span>

namespace mvk::detail
{

void
submit_draw_commands(types::queue graphics_queue,
                     types::command_buffer command_buffer,
                     types::semaphore const & image_available,
                     types::semaphore const & render_finished,
                     types::fence & frame_in_flight_fence) noexcept;

void
stage(types::device const & device, types::physical_device physical_device,
      types::queue graphics_queue, types::command_pool const & command_pool,
      types::buffer const & buffer, utility::slice<std::byte> src,
      types::device_size offset);

[[nodiscard]] std::span<std::byte>
map_memory(types::device_memory const & memory,
           types::device_size size = VK_WHOLE_SIZE,
           types::device_size offset = 0) noexcept;

void
transition_layout(types::queue graphics_queue,
                  types::command_pool const & command_pool,
                  types::image const & image, VkImageLayout old_layout,
                  VkImageLayout new_layout, uint32_t mipmap_levels) noexcept;

void
stage(types::device const & device, types::physical_device physical_device,
      types::queue graphics_queue, types::command_pool const & command_pool,
      types::image const & buffer, utility::slice<std::byte> src,
      uint32_t width, uint32_t height) noexcept;

void
generate_mipmaps(types::queue graphics_queue,
                 types::command_pool const & command_pool,
                 types::image const & image, uint32_t width, uint32_t height,
                 uint32_t mipmap_levels);

[[nodiscard]] std::tuple<std::vector<unsigned char>, uint32_t, uint32_t>
load_texture(std::filesystem::path const & path);

[[nodiscard]] std::pair<types::buffer, types::device_memory>
create_staging_buffer_and_memory(types::device const & device,
                                 types::physical_device physical_device,
                                 utility::slice<std::byte> src) noexcept;

[[nodiscard]] types::command_buffers
create_staging_command_buffer(types::command_pool const & pool) noexcept;

void
submit_staging_command_buffer(
    types::queue graphics_queue,
    types::command_buffers const & command_buffer) noexcept;

template <typename Checker>
requires result_checker<Checker>
void
present_swapchain(types::queue present_queue,
                  types::swapchain const & swapchain,
                  types::semaphore const & render_finished,
                  uint32_t image_index, Checker && check) noexcept;

template <typename Checker>
requires requirement_checker<Checker>
[[nodiscard]] static VkSurfaceFormatKHR
choose_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                      Checker && check) noexcept;

[[nodiscard]] std::optional<uint32_t>
find_memory_type(VkPhysicalDevice physical_device, uint32_t filter,
                 VkMemoryPropertyFlags properties_flags);

[[nodiscard]] std::optional<uint32_t>
next_swapchain_image(VkDevice device, VkSwapchainKHR swapchain,
                     VkSemaphore semaphore, VkFence fence);

[[nodiscard]] uint32_t
calculate_mimap_levels(uint32_t height, uint32_t width) noexcept;

} // namespace mvk::detail

namespace mvk::detail
{
template <typename Checker>
requires result_checker<Checker>
void
present_swapchain(types::queue const present_queue,
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

  auto const result =
      vkQueuePresentKHR(types::get(present_queue), &present_info);
  std::forward<Checker>(check)(result);
  vkQueueWaitIdle(types::get(present_queue));
}

template <typename Checker>
requires requirement_checker<Checker>
[[nodiscard]] static VkSurfaceFormatKHR
choose_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                      Checker && check) noexcept
{
  auto const formats = query<vkGetPhysicalDeviceSurfaceFormatsKHR>::with(
      physical_device, surface);
  auto const it = std::find_if(std::begin(formats), std::end(formats),
                               std::forward<Checker>(check));

  if (it != std::end(formats))
  {
    return *it;
  }

  return formats[0];
}

} // namespace mvk::detail

#endif
