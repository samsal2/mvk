#ifndef MVK_DETAIL_CREATORS_HPP_INCLUDED
#define MVK_DETAIL_CREATORS_HPP_INCLUDED

#include "types/types.hpp"

#include "detail/misc.hpp"

namespace mvk::detail
{

[[nodiscard]] types::instance
create_instance(types::window const & window,
                std::string const & name) noexcept;

[[nodiscard]] types::command_pool
create_command_pool(types::device const & device,
                    types::queue_index queue_index,
                    VkCommandPoolCreateFlags flags = 0) noexcept;

[[nodiscard]] types::command_buffers
create_command_buffers(types::command_pool const & pool, uint32_t count,
                       VkCommandBufferLevel level) noexcept;

[[nodiscard]] types::device_memory
create_device_memory(types::physical_device physical_device,
                     types::buffer const & buffer,
                     VkMemoryPropertyFlags properties) noexcept;

[[nodiscard]] types::device_memory
create_device_memory(types::physical_device physical_device,
                     types::image const & buffer,
                     VkMemoryPropertyFlags properties) noexcept;

[[nodiscard]] types::shader_module
create_shader_module(types::device const & device,
                     utility::slice<char> code) noexcept;

} // namespace mvk::detail

#endif
