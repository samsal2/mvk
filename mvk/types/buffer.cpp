#include "types/buffer.hpp"

#include "types/command_buffers.hpp"
#include "types/command_pool.hpp"
#include "types/detail/staging.hpp"
#include "types/device.hpp"
#include "types/device_memory.hpp"

namespace mvk::types
{

buffer::buffer(VkDevice const device, VkBufferCreateInfo const & info)
    : wrapper(nullptr, device)
{
  [[maybe_unused]] auto const result =
      vkCreateBuffer(parent(), &info, nullptr, &reference());
  MVK_VERIFY(VK_SUCCESS == result);
  vkGetBufferMemoryRequirements(parent(), get(), &memory_requirements_);
}

buffer &
buffer::stage(device const & device, command_pool const & command_pool,
              utility::slice<std::byte> const data_source,
              VkDeviceSize const offset)
{
  auto const [staging_buffer, staging_buffer_memory] =
      detail::create_staging_buffer_and_memory(device, data_source);
  auto const staging_command_buffer =
      detail::create_staging_command_buffer(device, command_pool);

  auto const begin_info = []
  {
    auto info = VkCommandBufferBeginInfo();
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;
    return info;
  }();

  auto const copy_region = [staging_buffer = &staging_buffer, offset]
  {
    auto region = VkBufferCopy();
    region.srcOffset = 0;
    region.dstOffset = offset;
    region.size = staging_buffer->size();
    return region;
  }();

  staging_command_buffer.begin(0, begin_info)
      .copy_buffer({staging_buffer.get(), get()}, {&copy_region, 1})
      .end();

  detail::submit_staging_command_buffer(device, staging_command_buffer);
  return *this;
}

} // namespace mvk::types
