#ifndef MVK_VK_TYPES_DETAIL_STAGING_HPP_INCLUDED
#define MVK_VK_TYPES_DETAIL_STAGING_HPP_INCLUDED

#include "utility/slice.hpp"
#include "utility/types.hpp"
#include "vk_types/buffer.hpp"
#include "vk_types/command_buffers.hpp"
#include "vk_types/command_pool.hpp"
#include "vk_types/common.hpp"
#include "vk_types/device.hpp"
#include "vk_types/device_memory.hpp"

namespace mvk::vk_types::detail
{

[[nodiscard]] std::pair<buffer, device_memory>
create_staging_buffer_and_memory(
  device const &            device,
  utility::slice<std::byte> data_source);

[[nodiscard]] command_buffers
create_staging_command_buffer(
  device const &       device,
  command_pool const & command_pool);

void
submit_staging_command_buffer(
  device const &          device,
  command_buffers const & command_buffer);

} // namespace mvk::vk_types::detail

#endif
