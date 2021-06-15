#ifndef MVK_TYPES_DETAIL_STAGING_HPP_INCLUDED
#define MVK_TYPES_DETAIL_STAGING_HPP_INCLUDED

#include "types/buffer.hpp"
#include "types/command_buffers.hpp"
#include "types/command_pool.hpp"
#include "types/common.hpp"
#include "types/device.hpp"
#include "types/device_memory.hpp"
#include "utility/slice.hpp"
#include "utility/types.hpp"

namespace mvk::types::detail
{

[[nodiscard]] std::pair<buffer, device_memory>
create_staging_buffer_and_memory(device const & device,
                                 utility::slice<std::byte> src) noexcept;

[[nodiscard]] command_buffers
create_staging_command_buffer(device const & device,
                              command_pool const & command_pool) noexcept;

void
submit_staging_command_buffer(
    device const & device, command_buffers const & command_buffer) noexcept;

} // namespace mvk::types::detail

#endif
