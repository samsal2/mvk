#ifndef MVK_DETAIL_MISC_HPP_INCLUDED
#define MVK_DETAIL_MISC_HPP_INCLUDED

#include "detail/checkers.hpp"
#include "types/types.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"

#include <filesystem>
#include <optional>

namespace mvk::detail
{
void
submit_draw_commands(types::device device, types::queue graphics_queue,
                     types::command_buffer command_buffer,
                     types::semaphore image_available,
                     types::semaphore render_finished,
                     types::fence frame_in_flight_fence) noexcept;

void
stage(types::device device, types::physical_device physical_device,
      types::queue graphics_queue, types::command_pool command_pool,
      types::buffer buffer, utility::slice<std::byte const> src,
      types::device_size offset);

[[nodiscard]] utility::slice<std::byte>
map_memory(types::device device, types::device_memory memory,
           types::device_size size = VK_WHOLE_SIZE,
           types::device_size offset = 0) noexcept;

void
transition_layout(types::device device, types::queue graphics_queue,
                  types::command_pool command_pool, types::image image,
                  VkImageLayout old_layout, VkImageLayout new_layout,
                  uint32_t mipmap_levels) noexcept;

void
stage(types::device device, types::physical_device physical_device,
      types::queue graphics_queue, types::command_pool command_pool,
      types::image buffer, utility::slice<std::byte const> src,
      uint32_t width, uint32_t height) noexcept;

void
generate_mipmaps(types::device device, types::queue graphics_queue,
                 types::command_pool command_pool, types::image image,
                 uint32_t width, uint32_t height, uint32_t mipmap_levels);

[[nodiscard]] std::tuple<std::vector<unsigned char>, uint32_t, uint32_t>
load_texture(std::filesystem::path const & path);

[[nodiscard]] std::pair<types::unique_buffer, types::unique_device_memory>
create_staging_buffer_and_memory(
    types::device device, types::physical_device physical_device,
    utility::slice<std::byte const> src) noexcept;

[[nodiscard]] types::unique_command_buffer
create_staging_command_buffer(types::device,
                              types::command_pool pool) noexcept;

void
submit_staging_command_buffer(types::queue graphics_queue,
                              types::command_buffer command_buffer) noexcept;

template <typename Checker>
requires result_checker<Checker>
void
present_swapchain(types::queue present_queue, types::swapchain swapchain,
                  types::semaphore render_finished, uint32_t image_index,
                  Checker && check) noexcept;

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
                  types::swapchain const swapchain,
                  types::semaphore const render_finished,
                  uint32_t const image_index, Checker && check) noexcept
{
  auto const signal_semaphores = std::array{types::get(render_finished)};
  auto const swapchains = std::array{types::get(swapchain)};
  auto const image_indices = std::array{image_index};

  auto const present_info = [&signal_semaphores, &swapchains, &image_indices]
  {
    auto info = VkPresentInfoKHR();
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount =
        static_cast<uint32_t>(std::size(signal_semaphores));
    info.pWaitSemaphores = std::data(signal_semaphores);
    info.swapchainCount = static_cast<uint32_t>(std::size(swapchains));
    info.pSwapchains = std::data(swapchains);
    info.pImageIndices = std::data(image_indices);
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
  auto formats_count = uint32_t(0);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
                                       &formats_count, nullptr);

  auto formats = std::vector<VkSurfaceFormatKHR>(formats_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
                                       &formats_count, std::data(formats));

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
