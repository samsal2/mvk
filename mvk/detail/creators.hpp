#ifndef MVK_DETAIL_CREATORS_HPP_INCLUDED
#define MVK_DETAIL_CREATORS_HPP_INCLUDED

#include "types/types.hpp"

namespace mvk::detail
{

template <typename Buffer>
[[nodiscard]] types::device_memory
create_device_memory(types::device const & device, Buffer const & buffer,
                     VkMemoryPropertyFlags properties);

[[nodiscard]] types::shader_module
create_shader_module(types::device const & device, utility::slice<char> code);

} // namespace mvk::detail

namespace mvk::detail
{

template <typename Buffer>
[[nodiscard]] types::device_memory
create_device_memory(types::device const & device, Buffer const & buffer,
                     VkMemoryPropertyFlags const properties)
{
  auto const requirements = buffer.memory_requirements();
  auto const memory_type_index = types::detail::find_memory_type(
      device.physical_device(), requirements.memoryTypeBits, properties);

  MVK_VERIFY(memory_type_index.has_value());

  auto allocate_info = VkMemoryAllocateInfo();
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = requirements.size;
  allocate_info.memoryTypeIndex = memory_type_index.value();

  auto tmp = types::device_memory(device.get(), allocate_info);
  tmp.bind(buffer);
  return tmp;
}

} // namespace mvk::detail

#endif
