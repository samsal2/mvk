#include "types/command_buffers.hpp"

#include <array>

namespace mvk::types
{

command_buffers::command_buffers(VkDevice const device,
                                 VkCommandBufferAllocateInfo const & info)
    : wrapper({info.commandBufferCount, nullptr}, device, info.commandPool)
{
  [[maybe_unused]] auto const result =
      vkAllocateCommandBuffers(parent(), &info, std::data(reference()));

  MVK_VERIFY(VK_SUCCESS == result);
}

[[nodiscard]] single_command_buffer
command_buffers::begin(
    size_t const index,
    VkCommandBufferBeginInfo const & begin_info) const noexcept
{
  auto const current_command_buffer = get()[index];
  vkBeginCommandBuffer(current_command_buffer, &begin_info);
  return single_command_buffer(current_command_buffer);
}

} // namespace mvk::types
