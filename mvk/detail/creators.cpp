#include "detail/creators.hpp"

namespace mvk::detail
{

[[nodiscard]] types::shader_module create_shader_module(types::device const & device, utility::slice<char> const code) noexcept
{
        auto info     = VkShaderModuleCreateInfo();
        info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = static_cast<uint32_t>(std::size(code));
        info.pCode    = reinterpret_cast<uint32_t const *>(std::data(code));
        return {device.get(), info};
}

[[nodiscard]] types::device_memory create_device_memory(types::physical_device const physical_device, types::buffer const & buffer, VkMemoryPropertyFlags const properties) noexcept
{
        auto const requirements      = query<vkGetBufferMemoryRequirements>::with(types::parent(buffer), types::get(buffer));
        auto const memory_type_index = find_memory_type(types::get(physical_device), requirements.memoryTypeBits, properties);

        MVK_VERIFY(memory_type_index.has_value());

        auto allocate_info            = VkMemoryAllocateInfo();
        allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize  = requirements.size;
        allocate_info.memoryTypeIndex = memory_type_index.value();

        auto tmp = types::device_memory(types::parent(buffer), allocate_info);
        vkBindBufferMemory(types::parent(tmp), types::get(buffer), types::get(tmp), 0);
        return tmp;
}

[[nodiscard]] types::device_memory create_device_memory(types::physical_device const physical_device, types::image const & buffer, VkMemoryPropertyFlags const properties) noexcept
{
        auto const requirements      = query<vkGetImageMemoryRequirements>::with(types::parent(buffer), types::get(buffer));
        auto const memory_type_index = find_memory_type(types::get(physical_device), requirements.memoryTypeBits, properties);

        MVK_VERIFY(memory_type_index.has_value());

        auto allocate_info            = VkMemoryAllocateInfo();
        allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize  = requirements.size;
        allocate_info.memoryTypeIndex = memory_type_index.value();

        auto tmp = types::device_memory(types::parent(buffer), allocate_info);
        vkBindImageMemory(types::parent(tmp), types::get(buffer), types::get(tmp), 0);
        return tmp;
}

} // namespace mvk::detail
