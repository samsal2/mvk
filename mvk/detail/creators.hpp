#ifndef MVK_DETAIL_CREATORS_HPP_INCLUDED
#define MVK_DETAIL_CREATORS_HPP_INCLUDED

#include "detail/misc.hpp"
#include "types/types.hpp"

namespace mvk::detail
{
[[nodiscard]] types::unique_instance
create_instance(types::window const & window,
                std::string const & name) noexcept;

[[nodiscard]] types::unique_command_pool
create_command_pool(types::device device, types::queue_index queue_index,
                    VkCommandPoolCreateFlags flags = 0) noexcept;

[[nodiscard]] std::vector<types::unique_command_buffer>
create_command_buffers(types::device device, types::command_pool pool,
                       uint32_t count, VkCommandBufferLevel level) noexcept;

[[nodiscard]] types::unique_device_memory
create_device_memory(types::device device,
                     types::physical_device physical_device,
                     types::buffer buffer,
                     VkMemoryPropertyFlags properties) noexcept;

[[nodiscard]] types::unique_device_memory
create_device_memory(types::device device,
                     types::physical_device physical_device,
                     types::image buffer,
                     VkMemoryPropertyFlags properties) noexcept;

[[nodiscard]] types::unique_shader_module
create_shader_module(types::device device,
                     utility::slice<char const> code) noexcept;

} // namespace mvk::detail

#endif
