#ifndef MVK_DETAIL_CREATORS_HPP_INCLUDED
#define MVK_DETAIL_CREATORS_HPP_INCLUDED

#include "types/types.hpp"

#include "detail/misc.hpp"

namespace mvk::detail
{

[[nodiscard]] types::unique_instance
create_instance(types::window const & window,
                std::string const & name) noexcept;

[[nodiscard]] types::unique_command_pool
create_command_pool(types::unique_device const & device,
                    types::queue_index queue_index,
                    VkCommandPoolCreateFlags flags = 0) noexcept;

[[nodiscard]] std::vector<types::unique_command_buffer>
create_command_buffers(types::unique_command_pool const & pool,
                       uint32_t count, VkCommandBufferLevel level) noexcept;

[[nodiscard]] types::unique_device_memory
create_device_memory(types::physical_device physical_device,
                     types::unique_buffer const & buffer,
                     VkMemoryPropertyFlags properties) noexcept;

[[nodiscard]] types::unique_device_memory
create_device_memory(types::physical_device physical_device,
                     types::unique_image const & buffer,
                     VkMemoryPropertyFlags properties) noexcept;

[[nodiscard]] types::unique_shader_module
create_shader_module(types::unique_device const & device,
                     utility::slice<char> code) noexcept;

} // namespace mvk::detail

#endif
