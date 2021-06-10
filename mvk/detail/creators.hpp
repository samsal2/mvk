#ifndef MVK_DETAIL_CREATORS_HPP_INCLUDED
#define MVK_DETAIL_CREATORS_HPP_INCLUDED

#include "vk_types/vk_types.hpp"

namespace mvk::detail
{

template <typename T>
[[nodiscard]] vk_types::device_memory
create_device_memory(vk_types::device const & device, T const & buffer, VkMemoryPropertyFlags properties);

[[nodiscard]] vk_types::shader_module
create_shader_module(vk_types::device const & device, utility::slice<char> const code);

} // namespace mvk::detail

namespace mvk::detail
{

template <typename Buffer>
[[nodiscard]] vk_types::device_memory
create_device_memory(vk_types::device const & device, Buffer const & buffer, VkMemoryPropertyFlags const properties)
{
        auto const requirements = buffer.memory_requirements();

        using vk_types::detail::find_memory_type;
        auto const memory_type_index = find_memory_type(device.physical_device(), requirements.memoryTypeBits, properties);

        MVK_VERIFY(memory_type_index.has_value());

        auto allocate_info            = VkMemoryAllocateInfo();
        allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize  = requirements.size;
        allocate_info.memoryTypeIndex = memory_type_index.value();

        auto device_memory_tmp = vk_types::device_memory(device.get(), allocate_info);
        device_memory_tmp.bind(buffer);

        return device_memory_tmp;
}

} // namespace mvk::detail

#endif
