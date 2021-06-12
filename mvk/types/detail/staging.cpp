#include "types/detail/staging.hpp"

#include "types/command_buffers.hpp"
#include "types/detail/misc.hpp"
#include "utility/slice.hpp"
#include "utility/verify.hpp"

namespace mvk::types::detail
{

[[nodiscard]] std::pair<buffer, device_memory>
create_staging_buffer_and_memory(device const & device, utility::slice<std::byte> const data_source)
{
    auto const size = std::size(data_source);

    auto staging_buffer = [size, &device]
    {
        auto info        = VkBufferCreateInfo();
        info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size        = size;
        info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        return buffer(device.get(), info);
    }();

    auto buffer_memory = [&device, &staging_buffer]
    {
        auto const requirements           = staging_buffer.memory_requirements();
        auto const physical_device_handle = device.physical_device();
        auto const type_bits              = requirements.memoryTypeBits;

        auto const properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        auto const memory_type_index = find_memory_type(physical_device_handle, type_bits, properties);

        MVK_VERIFY(memory_type_index.has_value());

        auto info            = VkMemoryAllocateInfo();
        info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize  = requirements.size;
        info.memoryTypeIndex = memory_type_index.value();

        return device_memory(device.get(), info);
    }();

    buffer_memory.bind(staging_buffer).map(size).copy_data(data_source).unmap();
    return {std::move(staging_buffer), std::move(buffer_memory)};
}

[[nodiscard]] command_buffers
create_staging_command_buffer(device const & device, command_pool const & command_pool)
{
    auto info               = VkCommandBufferAllocateInfo();
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = command_pool.get();
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    return {device.get(), info};
}

void
submit_staging_command_buffer(device const & device, command_buffers const & command_buffer)
{

    auto const command_buffer_count = static_cast<uint32_t>(std::size(command_buffer.get()));

    auto submit_info               = VkSubmitInfo();
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = command_buffer_count;
    submit_info.pCommandBuffers    = std::data(command_buffer.get());

    auto [graphics_queue, present_queue] = device.get_queues();
    graphics_queue.submit(submit_info).wait_idle();
}

} // namespace mvk::types::detail
